# $PIP_VERSION:$
# $PIP_license:$
# $RIKEN_copyright:$

top_builddir = ../..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk
include $(srcdir)/../Test.common

LDFLAGS += -pie

SRCS    = a.f fort.c

TASKS = a b c
PROGRAMS = $(TASKS) fort

PROGRAMS_TO_INSTALL = # nothing

include $(top_srcdir)/build/rule.mk

b: a
	cp $< $@

c: a
	cp $< $@

post-clean-hook:
	$(RM) *.E *.log
