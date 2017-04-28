# $PIP_VERSION:$
# $PIP_license:$
# $RIKEN_copyright:$

top_builddir = ../..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk
include $(srcdir)/../Test.common

SRCS    = pthread.c

DEPINCS = ../test.h $(PIPINCDIR)/pip.h $(PIPINCDIR)/pip_machdep.h

PROGRAM = pthread

PROGRAMS_TO_INSTALL = # nothing

include $(top_srcdir)/build/rule.mk

post-clean-hook:
	$(RM) *.E *.log
