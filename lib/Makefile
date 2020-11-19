# $PIP_VERSION: Version 1.2.0$
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

top_builddir = ..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk

LIBNAME  = libpip.so
LIBRARY_PIPGDB = libpip_gdb.so

CPPFLAGS += -I$(PIPINCDIR) -I. \
	-DPIP_INSTALL_LIBPIPGDB=\"$(default_libdir)/$(LIBRARY_PIPGDB)\" \
	-DPIP_MAKE_LIBPIPGDB=\"`pwd`/$(LIBRARY_PIPGDB)\"
CFLAGS += $(PICFLAG)
LDFLAGS  = -shared -Wl,-soname -Wl,$(LIBRARY) -L$(glibc_libdir) -ldl

SO_MAJOR = 0
LIBRARY  = $(LIBNAME).$(SO_MAJOR)

LIBRARIES = $(LIBRARY) $(LIBRARY_PIPGDB)

SRCS     = pip.c pip_util.c pip_gdb.c
OBJS	 = pip.o pip_util.o
OBJS_PIPGDB = pip_gdb.o

DEPINCS  = $(PIPINCDIR)/pip.h			\
	   $(PIPINCDIR)/pip_clone.h		\
	   $(PIPINCDIR)/pip_debug.h		\
	   $(PIPINCDIR)/pip_gdbif.h		\
	   $(PIPINCDIR)/pip_internal.h 		\
	   $(PIPINCDIR)/pip_machdep.h 		\
	   $(PIPINCDIR)/pip_machdep_aarch64.h 	\
	   $(PIPINCDIR)/pip_machdep_x86_64.h 	\
	   $(PIPINCDIR)/pip_queue.h 		\
	   $(PIPINCDIR)/pip_ulp.h 		\
	   $(PIPINCDIR)/pip_util.h

include $(top_srcdir)/build/rule.mk

$(OBJS): $(DEPINCS) Makefile

$(LIBRARY): $(OBJS) Makefile
	$(CC) $(LDFLAGS) $(OBJS) -o $@
	rm -f $(LIBNAME)
	ln -s $(LIBRARY) $(LIBNAME)

$(LIBRARY_PIPGDB): $(OBJS_PIPGDB) Makefile
	$(CC) $(LDFLAGS) $(OBJS_PIPGDB) -o $@

# chmod is to make rpmbuild happy
install:
	chmod 0755 $(DESTDIR)$(libdir)/$(LIBRARY)
	rm -f $(DESTDIR)$(libdir)/$(LIBNAME)
	ln -s $(LIBRARY) $(DESTDIR)$(libdir)/$(LIBNAME)

very-clean:
	rm -f $(LIBNAME)

debug:
	CPPFLAGS="-DDEBUG" make all;
