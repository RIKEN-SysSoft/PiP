# $PIP_VERSION:$
# $PIP_license:$
# $RIKEN_copyright:$

top_builddir = ../..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk
include $(srcdir)/../Test.common

OMPFLAG = -fopenmp

CFLAGS += $(OMPFLAG)
LDFLAGS += $(OMPFLAG) -openmp

SRCS  = openmp.c

PROGRAMS  = openmp

PROGRAMS_TO_INSTALL = # nothing

include $(top_srcdir)/build/rule.mk

%: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -fpie -pie $< -o $@
