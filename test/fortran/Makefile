# $PIP_VERSION:$
# $PIP_license:$
# $RIKEN_copyright:$

top_builddir = ../..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk
include $(srcdir)/../Test.common

LDFLAGS += -pie

SRCS    = a.c spawns.c spawnp.c

TASKS = a b c
PROGRAMS = $(TASKS) fort

PROGRAMS_TO_INSTALL = # nothing

include $(top_srcdir)/build/rule.mk

b: a
	cp $< $@

c: a
	cp $< $@

%: %.c $(DEPINCS) $(PIPLIB) Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $< -o $@

%: %.f $(DEPINCS) $(PIPLIB) Makefile
	$(FC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $< -o $@

post-clean-hook:
	$(RM) *.E *.log
