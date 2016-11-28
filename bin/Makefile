# $PVAS_VERSION:$
# $PVAS_license:$
# $RIKEN_copyright:$

top_builddir = ..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk

PROGRAM = piprun

CFLAGS += -pthread
CPPFLAGS += -I$(PIPINCDIR) -DPROGRAM=\"$(PROGRAM)\"
LDLIBS += $(PIPLDLIB) -ldl

DEPINCS = $(PIPINCDIR)/pip.h

include $(top_srcdir)/build/rule.mk

$(PROGRAM): piprun.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $< $(LDLIBS) -o $@

post-clean-hook:
	$(RM) *.E *.log
