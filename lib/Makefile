
# $PIP_license: <Simplified BSD License>
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#     Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#
#     Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.
# $
# $RIKEN_copyright: Riken Center for Computational Sceience (R-CCS),
# System Software Development Team, 2016-2020
# $
# $PIP_VERSION: Version 3.0.0$
#
# $Author: Atsushi Hori (R-CCS) mailto: ahori@riken.jp or ahori@me.com
# $

top_builddir = ..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk
include fctx_defs.mk

LIBNAME  = libpip.so
SO_MAJOR = 0
LIBRARY  = $(LIBNAME).$(SO_MAJOR)

LIBNAME_PIPINIT = libpip_init.so
LIBRARY_PIPINIT = $(LIBNAME_PIPINIT).$(SO_MAJOR)
LIBRARIES = $(LIBRARY) $(LIBRARY_PIPINIT)

INTERP = $(dynamic_linker)

CPPFLAGS += $(PIP_CPPFLAGS) $(FCTXCPPFLAG) -DINTERP=\"$(INTERP)\" \
	-DPIP_INSTALL_GLIBCDIR=\"$(glibc_libdir)\" \
	-DPIP_INSTALL_GLIBC=\"$(glibc_lib)\" \
	-DPIP_INSTALL_LIBPIPINIT=\"$(default_libdir)/$(LIBRARY_PIPINIT)\" \
	-DPIP_BUILD_LIBPIPINIT=\"`pwd`/$(LIBRARY_PIPINIT)\" \
	-DPIP_INSTALL_BIN_PIPS=\"$(default_bindir)/pips\" \
	-DPIP_BUILD_BIN_PIPS=\"../bin/pips\" \
	-DPREFIX=\"$(prefix)\" \
	-DLDLINUX=\"$(dynamic_linker)\" \
	$(CPPFLAGS_DEBUG)
CFLAGS   += $(PIP_CPPFLAGS) $(PIC_CFLAG) $(PTHREAD_FLAG)
LDFLAGS  += -shared -Wl,-soname -Wl,$(LIBRARY) $(PTHREAD_FLAG)

LIBMAIN = -Wl,-e,pip_main

SRCS  = pip_main.c pip.c pip_spawn.c pip_wait.c pip_got.c pip_clone.c \
	pip_blt.c pip_sync.c pip_util.c pip_gdbif.c pip_namexp.c \
	pip_dlfcn.c pip_signal.c pip_init.c pip_context.c

OBJS_PIPINIT = pip_init.o

OBJS = pip_main.o pip.o pip_spawn.o pip_wait.o pip_got.o pip_clone.o \
	pip_blt.o pip_sync.o pip_util.o pip_gdbif.o pip_namexp.o \
	pip_dlfcn.o pip_signal.o $(FCTXOBJS) $(OBJS_PIPINIT) \
	pip_context.o

all: $(LIBRARIES)

include $(top_srcdir)/build/rule.mk
include fcontext.mk

$(PIP_INCDIR)/build.h: FORCE
	./build-header.sh $(CC) $(PIP_INCDIR)/build.h
.PHONY: FORCE

concat: concat.c $(DEPINCS)
	$(CC) -I$(PIP_INCDIR) -O concat.c -o concat

$(PIP_INCDIR)/concat.h: concat
	./concat > $(PIP_INCDIR)/concat.h

$(OBJS): $(PIP_INCDIR)/build.h $(PIP_INCDIR)/concat.h

$(LIBRARY): $(OBJS) $(DEPINCS) Makefile
	$(CC) $(LDFLAGS) $(LIBMAIN) $(OBJS) -o $@
	$(RM) $(LIBNAME)
	ln -s $(LIBRARY) $(LIBNAME)

$(LIBRARY_PIPINIT): $(OBJS_PIPINIT) $(DEPINCS) Makefile
	$(CC) $(LDFLAGS) $(OBJS_PIPINIT) -o $@
	rm -f $(LIBNAME_PIPINIT)
	ln -s $(LIBRARY_PIPINIT) $(LIBNAME_PIPINIT)

# chmod is to make rpmbuild happy
install:
	chmod 0755 $(libdir)/$(LIBRARY) $(libdir)/$(LIBRARY_PIPINIT)
	$(RM) $(libdir)/$(LIBNAME) $(libdir)/$(LIBNAME_PIPINIT)
	ln -s $(LIBRARY) $(libdir)/$(LIBNAME)
	ln -s $(LIBRARY_PIPINIT) $(libdir)/$(LIBNAME_PIPINIT)

debug:
	CPPFLAGS_DEBUG="-DDEBUG" $(MAKE) all;

cdebug:
	$(MAKE) clean debug;

TAGS:
	ctags -Re

post-clean-hook:
	$(RM) $(OBJS_PIPINIT) $(LIBNAME) $(LIBNAME_PIPINIT)

post-veryclean-hook:
	$(RM) $(PIP_INCDIR)//build.h $(PIP_INCDIR)//concat.h concat
