# $PIP_VERSION: Version 2.0.0$
# $PIP_license: <Simplified BSD License>
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# The views and conclusions contained in the software and documentation
# are those of the authors and should not be interpreted as representing
# official policies, either expressed or implied, of the PiP project.$
# $RIKEN_copyright: Riken Center for Computational Sceience,
# System Software Development Team, 2016, 2017, 2018, 2019, 2020$

top_builddir = ..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk

LIBNAME  = libpip.so
SO_MAJOR = 0
LIBRARY  = $(LIBNAME).$(SO_MAJOR)

LIBNAME_PIPINIT = libpip_init.so
LIBRARY_PIPINIT = $(LIBNAME_PIPINIT).$(SO_MAJOR)
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
CFLAGS   += $(PIP_CPPFLAGS) $(PIC_CFLAG) $(PTHREADFLAG)
LDFLAGS  += -shared -Wl,-soname -Wl,$(LIBRARY) $(PTHREADFLAG)

LIBMAIN = -Wl,-e,pip_main

SRCS = pip_main.c pip.c pip_init.c pip_util.c pip_gdb.c pip_2_backport.c pip_signal.c \
	pip_wait.c pip_dlfcn.c pip_gdbif.c pip_got.c pip_clone.c pip_namexp.c

OBJS_PIPINIT = pip_init.o

OBJS	 = pip_main.o pip.o pip_util.o pip_2_backport.o pip_signal.o pip_wait.o \
	pip_dlfcn.o pip_gdbif.o pip_got.o pip_clone.o pip_namexp.o $(OBJS_PIPINIT)

DEPINCS =  $(PIP_INCDIR)/pip_config.h		\
	   $(PIP_INCDIR)/pip.h			\
	   $(PIP_INCDIR)/pip_internal.h		\
	   $(PIP_INCDIR)/pip_clone.h		\
	   $(PIP_INCDIR)/pip_dlfcn.h		\
	   $(PIP_INCDIR)/pip_debug.h		\
	   $(PIP_INCDIR)/pip_gdbif.h		\
	   $(PIP_INCDIR)/pip_machdep.h 		\
	   $(PIP_INCDIR)/pip_machdep_aarch64.h 	\
	   $(PIP_INCDIR)/pip_machdep_x86_64.h 	\
	   $(PIP_INCDIR)/pip_gdbif.h		\
	   $(PIP_INCDIR)/pip_gdbif_func.h	\
	   $(PIP_INCDIR)/pip_gdbif_queue.h	\
	   $(PIP_INCDIR)/pip_queue.h 		\
	   $(PIP_INCDIR)/pip_util.h		\
	   $(PIP_INCDIR)/pip_debug.h

include $(top_srcdir)/build/rule.mk

$(PIP_INCDIR)/build.h: FORCE
	@./build-header.sh $(CC) > $(PIP_INCDIR)/.build.h
	@if ! diff $(PIP_INCDIR)/.build.h $(PIP_INCDIR)/build.h > /dev/null 2>&1; then \
	    mv -f $(PIP_INCDIR)/.build.h $(PIP_INCDIR)/build.h; \
	fi
	@$(RM) $(PIP_INCDIR)/.build.h > /dev/null 2>&1
.PHONY: FORCE

$(OBJS): $(DEPINCS) $(PIP_INCDIR)/build.h Makefile

$(LIBRARY): $(OBJS) Makefile
	$(CC) $(LDFLAGS) $(LIBMAIN) $(OBJS) -o $@
	rm -f $(LIBNAME)
	ln -s $(LIBRARY) $(LIBNAME)

$(LIBRARY_PIPINIT): $(OBJS_PIPINIT) Makefile
	$(CC) $(LDFLAGS) $(OBJS_PIPINIT) -o $@
	rm -f $(LIBNAME_PIPINIT)
	ln -s $(LIBRARY_PIPINIT) $(LIBNAME_PIPINIT)

# chmod is to make rpmbuild happy
install:
	chmod 0755 $(libdir)/$(LIBRARY)
	$(RM) $(libdir)/$(LIBNAME)
	$(RM) $(libdir)/$(LIBNAME_PIPINIT)
	ln -s $(LIBRARY) $(libdir)/$(LIBNAME)
	ln -s $(LIBRARY_PIPINIT) $(libdir)/$(LIBNAME_PIPINIT)

debug:
	CPPFLAGS="-DDEBUG" $(MAKE) all;

cdebug: clean
	CPPFLAGS="-DDEBUG" $(MAKE) all;

TAGS:
	ctags -Re

post-clean-hook:
	$(RM) $(OBJS_PIPINIT) $(LIBNAME) $(LIBNAME_PIPINIT)
