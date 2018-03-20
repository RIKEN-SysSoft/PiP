# $PIP_VERSION:$
# $PIP_license:$
# $RIKEN_copyright:$

top_builddir = ../..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk
include $(srcdir)/../Test.common

PROGRAMS = dlmopen_count pip_mode ompnumthread
PROGRAMS_TO_INSTALL = # nothing
SRCS	= dlmopen_count.c pip_mode.c

all: $(PROGRAMS)

pip_mode: pip_mode.c Makefile

dlmopen_count: dlmopen_count.c Makefile

ompnumthread: ompnumthread.c Makefile
	$(CC) -fopenmp -openmp -O2 $< -o$@

include $(top_srcdir)/build/rule.mk
