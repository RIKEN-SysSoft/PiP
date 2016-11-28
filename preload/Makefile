# $PVAS_VERSION:$
# $PVAS_license:$
# $RIKEN_copyright:$

top_builddir = ..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk

CPPFLAGS = -I$(PIPINCDIR)
CFLAGS += $(PICFLAG)
LDFLAGS  = -shared

LIBRARY  = pip_preload.so
SRCS     = $(srcdir)/pip_preload.c

DEPINCS  = $(PIPINCDIR)/pip_debug.h

include $(top_srcdir)/build/rule.mk

$(LIBRARY): $(SRCS) $(DEPINCS) Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(SRCS) -o $(LIBRARY)
