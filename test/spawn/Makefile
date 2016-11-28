# $PVAS_VERSION:$
# $PVAS_license:$
# $RIKEN_copyright:$

top_builddir = ../..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk

CPPFLAGS = -I$(srcdir)/.. -I$(PIPINCDIR)
CCFLAGS += $(PIEFLAG)
LDFLAGS += -rdynamic $(PIPLDLIB)

SRCS    = a.c spawns.c spawnp.c

DEPINCS = ../test.h $(PIPINCDIR)/pip.h $(PIPINCDIR)/pip_machdep.h

TASKS = a.out b.out c.out
#PROGRAMS = $(TASKS) spawns spawnp
PROGRAMS = $(TASKS) spawns

PROGRAMS_TO_INSTALL = # nothing

include $(top_srcdir)/build/rule.mk

b.out: a.out
	cp $< $@

c.out: a.out
	cp $< $@

a.out: a.c $(DEPINCS) $(PIPLIB) Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $< -o $@

%: %.c $(DEPINCS) $(PIPLIB) Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $< -o $@

post-clean-hook:
	$(RM) *.E *.log
