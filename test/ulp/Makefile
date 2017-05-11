# $PIP_VERSION:$
# $PIP_license:$
# $RIKEN_copyright:$

top_builddir = ../..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk
include $(srcdir)/../Test.common

SRCS  = ulp0.c

PROGRAMS  = ulp0

PROGRAMS_TO_INSTALL = # nothing

include $(top_srcdir)/build/rule.mk

test-file:
	( export LD_PRELOAD=$(PIPDIR)/preload/pip_preload.so; ./file )

post-clean-hook:
	$(RM) *.E *.log
