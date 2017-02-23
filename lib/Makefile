# $PIP_VERSION:$
# $PIP_license:$
# $RIKEN_copyright:$

top_builddir = ..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk

CPPFLAGS = -I$(PIPINCDIR)
CFLAGS += $(PICFLAG)
LDFLAGS  = -pthread -shared -L$(glibc_libdir) -ldl

LIBRARY  = libpip.so
SRCS     = pip.c pvas.c xpmem.c

OBJS	 = pip.o pvas.o xpmem.o

DEPINCS  = $(PIPINCDIR)/pip.h 			\
	   $(PIPINCDIR)/pip_internal.h 		\
	   $(PIPINCDIR)/pip_machdep.h 		\
	   $(PIPINCDIR)/pip_machdep_x86_64.h 	\
	   $(PIPINCDIR)/pip_machdep_aarch64.h 	\
	   $(PIPINCDIR)/pvas.h 			\
	   $(PIPINCDIR)/pip_debug.h

include $(top_srcdir)/build/rule.mk

$(OBJS): $(DEPINCS) Makefile

$(LIBRARY): $(OBJS) Makefile
	$(CC) $(LDFLAGS) $(OBJS) -o $@
