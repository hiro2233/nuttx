############################################################################
# examples/ustream/Makefile
#
#   Copyright (C) 2015 Gregory Nutt. All rights reserved.
#   Author: Gregory Nutt <gnutt@nuttx.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
# 3. Neither the name NuttX nor the names of its contributors may be
#    used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
# OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
# AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
############################################################################

-include $(TOPDIR)/.config
-include $(TOPDIR)/Make.defs
include $(APPDIR)/Make.defs

# Basic Unix domain networking test

SERVER_ASRCS =
SERVER_AOBJS = $(SERVER_ASRCS:.S=$(OBJEXT))

SERVER_CSRCS = 
SERVER_COBJS = $(SERVER_CSRCS:.c=$(OBJEXT))

SERVER_MAINSRC = ustream_server.c
SERVER_MAINOBJ = $(SERVER_MAINSRC:.c=$(OBJEXT))

SERVER_SRCS = $(SERVER_ASRCS) $(SERVER_CSRCS) $(SERVER_MAINSRC)
SERVER_OBJS = $(SERVER_AOBJS) $(SERVER_COBJS)

SERVER_PROGNAME = server$(EXEEXT)

SERVER_APPNAME = server
SERVER_PRIORITY = SCHED_PRIORITY_DEFAULT
SERVER_STACKSIZE = 2048

CLIENT_ASRCS =
CLIENT_AOBJS = $(CLIENT_ASRCS:.S=$(OBJEXT))

CLIENT_CSRCS = 
CLIENT_COBJS = $(CLIENT_CSRCS:.c=$(OBJEXT))

CLIENT_MAINSRC = ustream_client.c
CLIENT_MAINOBJ = $(CLIENT_MAINSRC:.c=$(OBJEXT))

CLIENT_SRCS = $(CLIENT_ASRCS) $(CLIENT_CSRCS) $(CLIENT_MAINSRC)
CLIENT_OBJS = $(CLIENT_AOBJS) $(CLIENT_COBJS)

CLIENT_PROGNAME = server$(EXEEXT)

CLIENT_APPNAME = client
CLIENT_PRIORITY = SCHED_PRIORITY_DEFAULT
CLIENT_STACKSIZE = 2048

AOBJS = $(CLIENT_AOBJS) $(SERVER_AOBJS)
COBJS = $(CLIENT_COBJS) $(CLIENT_MAINOBJ) $(SERVER_COBJS) $(SERVER_MAINOBJ)

SRCS = $(CLIENT_SRCS) $(SERVER_SRCS)
OBJS = $(AOBJS) $(COBJS)

ifneq ($(CONFIG_BUILD_KERNEL),y)
  SERVER_OBJS += $(SERVER_MAINOBJ)
  CLIENT_OBJS += $(CLIENT_MAINOBJ)
endif

ifeq ($(CONFIG_WINDOWS_NATIVE),y)
  USTREAM_BIN = ..\..\libapps$(LIBEXT)
else
ifeq ($(WINTOOL),y)
  USTREAM_BIN = ..\\..\\libapps$(LIBEXT)
else
  USTREAM_BIN = ../../libapps$(LIBEXT)
endif
endif

ifeq ($(WINTOOL),y)
  INSTALL_DIR = "${shell cygpath -w $(BIN_DIR)}"
else
  INSTALL_DIR = $(BIN_DIR)
endif

# Common build

VPATH =

all: .built
.PHONY: clean depend distclean

$(AOBJS): %$(OBJEXT): %.S
	$(call ASSEMBLE, $<, $@)

$(COBJS): %$(OBJEXT): %.c
	$(call COMPILE, $<, $@)

.built: $(OBJS)
	$(call ARCHIVE, $(USTREAM_BIN), $(OBJS))
	$(Q) touch .built

ifeq ($(CONFIG_BUILD_KERNEL),y)

$(BIN_DIR)$(DELIM)$(CLIENT_PROGNAME): $(CLIENT_OBJS)
	@echo "LD: $(CLIENT_PROGNAME)"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) -o $(INSTALL_DIR)$(DELIM)$(CLIENT_PROGNAME) $(ARCHCRT0OBJ) $(CLIENT_MAINOBJ) $(LDLIBS)
	$(Q) $(NM) -u  $(INSTALL_DIR)$(DELIM)$(CLIENT_PROGNAME)

$(BIN_DIR)$(DELIM)$(SERVER_PROGNAME): $(OBJS) $(SERVER_MAINOBJ)
	@echo "LD: $(SERVER_PROGNAME)"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) -o $(INSTALL_DIR)$(DELIM)$(SERVER_PROGNAME) $(ARCHCRT0OBJ) $(SERVER_MAINOBJ) $(LDLIBS)
	$(Q) $(NM) -u  $(INSTALL_DIR)$(DELIM)$(SERVER_PROGNAME)

install: $(BIN_DIR)$(DELIM)$(CLIENT_PROGNAME) $(BIN_DIR)$(DELIM)$(SERVER_PROGNAME)

else
install:

endif

ifeq ($(CONFIG_NSH_BUILTIN_APPS),y)

$(BUILTIN_REGISTRY)$(DELIM)$(CLIENT_APPNAME)_main.bdat: $(DEPCONFIG) Makefile
	$(call REGISTER,$(CLIENT_APPNAME),$(CLIENT_PRIORITY),$(CLIENT_STACKSIZE),$(CLIENT_APPNAME)_main)

$(BUILTIN_REGISTRY)$(DELIM)$(SERVER_APPNAME)_main.bdat: $(DEPCONFIG) Makefile
	$(call REGISTER,$(SERVER_APPNAME),$(SERVER_PRIORITY),$(SERVER_STACKSIZE),$(SERVER_APPNAME)_main)

context: $(BUILTIN_REGISTRY)$(DELIM)$(CLIENT_APPNAME)_main.bdat $(BUILTIN_REGISTRY)$(DELIM)$(SERVER_APPNAME)_main.bdat

else

context:

endif

.depend: Makefile $(SERVER_SRCS)
	@$(MKDEP) $(ROOTDEPPATH) "$(CC)" -- $(CFLAGS) -- $(SERVER_SRCS) >Make.dep
	@touch $@

depend: .depend

clean:
	$(call DELFILE, .built)
	$(call DELFILE, *.dSYM)
	$(call CLEAN)

distclean: clean
	$(call DELFILE, Make.dep)
	$(call DELFILE, .depend)

-include Make.dep

.PHONY: preconfig
preconfig:
