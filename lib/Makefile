# $PIP_VERSION:$
# $PIP_license:$
# $RIKEN_copyright:$

top_builddir = ..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk

CPPFLAGS += -I$(PIPINCDIR)
CFLAGS += $(PICFLAG)
LDFLAGS  = -shared -L$(glibc_libdir) -ldl

LIBRARY  = libpip.so
SRCS     = pip.c pip_util.c pvas.c

OBJS	 = pip.o pip_util.o pvas.o

DEPINCS  = $(PIPINCDIR)/pip.h			\
	   $(PIPINCDIR)/pip_debug.h		\
	   $(PIPINCDIR)/pip_internal.h 		\
	   $(PIPINCDIR)/pip_machdep_aarch64.h 	\
	   $(PIPINCDIR)/pip_machdep.h 		\
	   $(PIPINCDIR)/pip_machdep_x86_64.h 	\
	   $(PIPINCDIR)/pip_ulp.h 		\
	   $(PIPINCDIR)/pip_util.h 		\
	   $(PIPINCDIR)/pvas.h

include $(top_srcdir)/build/rule.mk

$(OBJS): $(DEPINCS) Makefile

$(LIBRARY): $(OBJS) Makefile
	$(CC) $(LDFLAGS) $(OBJS) -o $@

debug:
	CPPFLAGS="-DDEBUG" make all;
