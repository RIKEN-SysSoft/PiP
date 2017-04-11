# $PIP_VERSION:$
# $PIP_license:$
# $RIKEN_copyright:$

top_builddir = ../..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk
include $(srcdir)/../Test.common

SRCS	= dlmopen_count.c pip_mode.c

all: dlmopen_count pip_mode ompnumthread

pip_mode: pip_mode.c Makefile

dlmopen_count: dlmopen_count.c Makefile

ompnumthread: ompnumthread.c Makefile
	$(CC) -fopenmp -openmp -O2 $< -o$@

PROGRAMS_TO_INSTALL = # nothing

include $(top_srcdir)/build/rule.mk
