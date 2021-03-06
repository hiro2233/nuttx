/****************************************************************************
 * fs/procfs/fs_procfskmm.c
 *
 *   Copyright (C) 2016 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/kmalloc.h>
#include <nuttx/mm/mm.h>
#include <nuttx/fs/fs.h>
#include <nuttx/fs/procfs.h>

#if defined(CONFIG_MM_KERNEL_HEAP) && defined(CONFIG_FS_PROCFS) && \
   !defined(CONFIG_FS_PROCFS_EXCLUDE_KMM)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Determines the size of an intermediate buffer that must be large enough
 * to handle the longest line generated by this logic.
 */

#define KMM_LINELEN 54

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* This structure describes one open "file" */

struct kmm_file_s
{
  struct procfs_file_s base;      /* Base open file structure */
  unsigned int linesize;          /* Number of valid characters in line[] */
  char line[KMM_LINELEN];         /* Pre-allocated buffer for formatted lines */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* File system methods */

static int     kmm_open(FAR struct file *filep, FAR const char *relpath,
                 int oflags, mode_t mode);
static int     kmm_close(FAR struct file *filep);
static ssize_t kmm_read(FAR struct file *filep, FAR char *buffer,
                 size_t buflen);
static int     kmm_dup(FAR const struct file *oldp,
                 FAR struct file *newp);
static int     kmm_stat(FAR const char *relpath, FAR struct stat *buf);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* See fs_mount.c -- this structure is explicitly externed there.
 * We use the old-fashioned kind of initializers so that this will compile
 * with any compiler.
 */

const struct procfs_operations kmm_operations =
{
  kmm_open,       /* open */
  kmm_close,      /* close */
  kmm_read,       /* read */
  NULL,           /* write */
  kmm_dup,        /* dup */
  NULL,           /* opendir */
  NULL,           /* closedir */
  NULL,           /* readdir */
  NULL,           /* rewinddir */
  kmm_stat        /* stat */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: kmm_open
 ****************************************************************************/

static int kmm_open(FAR struct file *filep, FAR const char *relpath,
                      int oflags, mode_t mode)
{
  FAR struct kmm_file_s *procfile;

  finfo("Open '%s'\n", relpath);

  /* PROCFS is read-only.  Any attempt to open with any kind of write
   * access is not permitted.
   *
   * REVISIT:  Write-able proc files could be quite useful.
   */

  if ((oflags & O_WRONLY) != 0 || (oflags & O_RDONLY) == 0)
    {
      ferr("ERROR: Only O_RDONLY supported\n");
      return -EACCES;
    }

  /* "kmm" is the only acceptable value for the relpath */

  if (strcmp(relpath, "kmm") != 0)
    {
      ferr("ERROR: relpath is '%s'\n", relpath);
      return -ENOENT;
    }

  /* Allocate a container to hold the file attributes */

  procfile = (FAR struct kmm_file_s *)kmm_zalloc(sizeof(struct kmm_file_s));
  if (!procfile)
    {
      ferr("ERROR: Failed to allocate file attributes\n");
      return -ENOMEM;
    }

  /* Save the attributes as the open-specific state in filep->f_priv */

  filep->f_priv = (FAR void *)procfile;
  return OK;
}

/****************************************************************************
 * Name: kmm_close
 ****************************************************************************/

static int kmm_close(FAR struct file *filep)
{
  FAR struct kmm_file_s *procfile;

  /* Recover our private data from the struct file instance */

  procfile = (FAR struct kmm_file_s *)filep->f_priv;
  DEBUGASSERT(procfile);

  /* Release the file attributes structure */

  kmm_free(procfile);
  filep->f_priv = NULL;
  return OK;
}

/****************************************************************************
 * Name: kmm_read
 ****************************************************************************/

static ssize_t kmm_read(FAR struct file *filep, FAR char *buffer,
                           size_t buflen)
{
  FAR struct kmm_file_s *procfile;
  struct mallinfo mem;
  size_t linesize;
  size_t copysize;
  size_t totalsize;
  off_t offset;

  finfo("buffer=%p buflen=%d\n", buffer, (int)buflen);

  DEBUGASSERT(filep != NULL && buffer != NULL && buflen > 0);
  offset = filep->f_pos;

  /* Recover our private data from the struct file instance */

  procfile = (FAR struct kmm_file_s *)filep->f_priv;
  DEBUGASSERT(procfile);

  /* The first line is the headers */

  linesize  = snprintf(procfile->line, KMM_LINELEN,
                       "             total       used       free    largest\n");
  copysize  = procfs_memcpy(procfile->line, linesize, buffer, buflen,
                            &offset);
  totalsize = copysize;

  if (totalsize < buflen)
    {
      buffer += copysize;
      buflen -= copysize;

      /* The second line is the memory data */

#ifdef CONFIG_CAN_PASS_STRUCTS
      mem = kmm_mallinfo();
#else
      (void)kmm_mallinfo(&mem);
#endif

      linesize   = snprintf(procfile->line, KMM_LINELEN,
                            "Mem:   %11d%11d%11d%11d\n",
                            mem.arena, mem.uordblks, mem.fordblks,
                            mem.mxordblk);
      copysize   = procfs_memcpy(procfile->line, linesize, buffer, buflen,
                                 &offset);
      totalsize += copysize;
    }

  /* Update the file offset */

  filep->f_pos += totalsize;
  return totalsize;
}

/****************************************************************************
 * Name: kmm_dup
 *
 * Description:
 *   Duplicate open file data in the new file structure.
 *
 ****************************************************************************/

static int kmm_dup(FAR const struct file *oldp, FAR struct file *newp)
{
  FAR struct kmm_file_s *oldattr;
  FAR struct kmm_file_s *newattr;

  finfo("Dup %p->%p\n", oldp, newp);

  /* Recover our private data from the old struct file instance */

  oldattr = (FAR struct kmm_file_s *)oldp->f_priv;
  DEBUGASSERT(oldattr);

  /* Allocate a new container to hold the task and attribute selection */

  newattr = (FAR struct kmm_file_s *)kmm_malloc(sizeof(struct kmm_file_s));
  if (!newattr)
    {
      ferr("ERROR: Failed to allocate file attributes\n");
      return -ENOMEM;
    }

  /* The copy the file attributes from the old attributes to the new */

  memcpy(newattr, oldattr, sizeof(struct kmm_file_s));

  /* Save the new attributes in the new file structure */

  newp->f_priv = (FAR void *)newattr;
  return OK;
}

/****************************************************************************
 * Name: kmm_stat
 *
 * Description: Return information about a file or directory
 *
 ****************************************************************************/

static int kmm_stat(FAR const char *relpath, FAR struct stat *buf)
{
  /* "kmm" is the only acceptable value for the relpath */

  if (strcmp(relpath, "kmm") != 0)
    {
      ferr("ERROR: relpath is '%s'\n", relpath);
      return -ENOENT;
    }

  /* "kmm" is the name for a read-only file */

  buf->st_mode    = S_IFREG | S_IROTH | S_IRGRP | S_IRUSR;
  buf->st_size    = 0;
  buf->st_blksize = 0;
  buf->st_blocks  = 0;
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#endif /* CONFIG_MM_KERNEL_HEAP && CONFIG_FS_PROCFS && !CONFIG_FS_PROCFS_EXCLUDE_KMM */
