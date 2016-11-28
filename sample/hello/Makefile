# $PVAS_VERSION:$
# $PVAS_license:$
# $RIKEN_copyright:$

top_builddir = ../..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk

PIPCFLAGS  = -I$(PIPINCDIR) $(CFLAGS) $(PIEFLAG) $(PIPINCFLAGS)

DEPINCS = $(PIPINCDIR)/pip.h

SRCS  = hello.c

PROGRAMS  = hello

PROGRAMS_TO_INSTALL = # nothing

include $(top_srcdir)/build/rule.mk

%: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) $(PIPCFLAGS) $(CFLAGS) $(PIPLDFLAGS) $< -o $@

post-clean-hook:
	$(RM) *.E *.log
