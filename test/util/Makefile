# $PIP_VERSION:$
# $PIP_license:$
# $RIKEN_copyright:$

top_builddir = ../..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk
include $(srcdir)/../Test.common

CFLAGS  += -fopenmp
LDFLAGS += -openmp

SRCS	= dlmopen_count.c pip_mode ompnumthread

PROGRAMS = dlmopen_count pip_mode ompnumthread

PROGRAMS_TO_INSTALL = # nothing

include $(top_srcdir)/build/rule.mk
