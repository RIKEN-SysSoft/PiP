
include ../../build/Make.common
include ../Test.common

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

PROGS  = initfin stack export environ malloc file wait signal exit \
	mutex barrier core numa hook spawn varvars

all: $(PROGS)

clean:
	rm -f $(PROGS) *.o *.E *~ *.log .nfs*

test-file:
	( export LD_PRELOAD=$(PIPDIR)/preload/pip_preload.so; ./file )
