# $PIP_VERSION:$
# $PIP_license:$
# $RIKEN_copyright:$

top_builddir = ../..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk
include $(srcdir)/../Test.common

SRCS  = initfin.c \
	stack.c \
	export.c \
	environ.c \
	malloc.c \
	file.c \
	wait.c \
	signal.c \
	exit.c \
	mutex.c \
	barrier.c \
	core.c \
	numa.c \
	hook.c \
	spawn.c \
	varvars.c

PROGRAMS  = initfin stack export environ malloc file wait signal exit \
	mutex barrier core numa hook spawn varvars

PROGRAMS_TO_INSTALL = # nothing

include $(top_srcdir)/build/rule.mk

test-file:
	( export LD_PRELOAD=$(PIPDIR)/preload/pip_preload.so; ./file )

post-clean-hook:
	$(RM) *.E *.log
