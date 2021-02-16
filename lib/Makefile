
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
# System Software Development Team, 2016-2021
# $
# $PIP_VERSION: Version 2.0.0$
#
# $Author: Atsushi Hori (R-CCS)
# Query:   procinproc-info@googlegroups.com
# User ML: procinproc-users@googlegroups.com
# $

top_builddir = ..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk

SO_MAJOR = 0
LIBNAME  = libpip.so
LIBRARY  = $(LIBNAME).$(SO_MAJOR)
LDFLAGS_PIP = -shared -Wl,-soname -Wl,$(LIBRARY) $(PTHREAD_FLAG)

SO_MAJOR_PIPINIT = 0
LIBNAME_PIPINIT = libpip_init.so
LIBRARY_PIPINIT = $(LIBNAME_PIPINIT).$(SO_MAJOR_PIPINIT)
LDFLAGS_PIPINIT = -shared $(DFLAGS_EXTRA) -Wl,-soname -Wl,$(LIBRARY_PIPINIT)\
	$(PTHREAD_FLAG)


LIBRARIES = $(LIBRARY) $(LIBRARY_PIPINIT)

INTERP = $(dynamic_linker)

CPPFLAGS += $(PIP_CPPFLAGS) -DINTERP=\"$(INTERP)\" \
	-DPIP_INSTALL_GLIBCDIR=\"$(glibc_libdir)\" \
	-DPIP_INSTALL_GLIBC=\"$(glibc_lib)\" \
	-DPIP_INSTALL_LIBPIPINIT=\"$(default_libdir)/$(LIBRARY_PIPINIT)\" \
	-DPIP_BUILD_LIBPIPINIT=\"`pwd`/$(LIBRARY_PIPINIT)\" \
	-DPIP_INSTALL_BIN_PIPS=\"$(default_bindir)/pips\" \
	-DPIP_BUILD_BIN_PIPS=\"../bin/pips\" \
	-DPREFIX=\"$(prefix)\" \
	-DLDLINUX=\"$(dynamic_linker)\" \
	$(CPPFLAGS_DEBUG)
CFLAGS   += $(PIC_CFLAG) $(PTHREAD_FLAG)

LIBMAIN = -Wl,-e,pip_main

SRCS  = pip_main.c pip.c pip_init.c pip_util.c pip_gdb.c pip_2_backport.c pip_signal.c \
	pip_wait.c pip_dlfcn.c pip_gdbif.c pip_got.c pip_clone.c pip_namexp.c

OBJS_PIPINIT = pip_init.o

OBJS = pip_main.o pip.o pip_util.o pip_2_backport.o pip_signal.o pip_wait.o \
	pip_dlfcn.o pip_gdbif.o pip_got.o pip_clone.o pip_namexp.o $(OBJS_PIPINIT)

include $(top_srcdir)/build/rule.mk

BUILD_H = $(top_srcdir)/include/pip/build.h

$(BUILD_H): $(top_srcdir)/include/pip/pip_config.h
	( cd ../include/pip ; $(MAKE) header )

$(OBJS): $(BUILD_H)

$(LIBRARY): $(OBJS) $(DEPINCS) Makefile
	$(CC) $(LDFLAGS_PIP) $(LIBMAIN) $(OBJS) -o $@
	$(RM) $(LIBNAME)
	ln -s $(LIBRARY) $(LIBNAME)

$(LIBRARY_PIPINIT): $(LIBRARY) $(OBJS_PIPINIT) $(DEPINCS) Makefile
	$(CC) $(LDFLAGS_PIPINIT) $(OBJS_PIPINIT) -o $@
	$(RM) $(LIBNAME_PIPINIT)
	ln -s $(LIBRARY_PIPINIT) $(LIBNAME_PIPINIT)

# chmod is to make rpmbuild happy
install:
	chmod 0755 $(DESTDIR)$(libdir)/$(LIBRARY) \
		$(DESTDIR)$(libdir)/$(LIBRARY_PIPINIT)
	$(RM) $(DESTDIR)$(libdir)/$(LIBNAME) \
		$(DESTDIR)$(libdir)/$(LIBNAME_PIPINIT)
	ln -s $(LIBRARY) $(DESTDIR)$(libdir)/$(LIBNAME)
	ln -s $(LIBRARY_PIPINIT) $(DESTDIR)$(libdir)/$(LIBNAME_PIPINIT)

debug:
	CPPFLAGS="-DDEBUG" $(MAKE) all;

cdebug: clean
	CPPFLAGS="-DDEBUG" $(MAKE) all;

TAGS:
	ctags -Re

post-clean-hook:
	$(RM) $(OBJS_PIPINIT) $(LIBNAME) $(LIBNAME_PIPINIT)
