
include ../../build/Make.common

CPPFLAGS = -I../ -I$(PIPINCDIR)
CCFLAGS += $(PIEFLAG)
LDFLAGS += -rdynamic $(PIPLIB)

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
	numa.c

DEPINCS = ../test.h $(PIPINCDIR)/pip.h $(PIPINCDIR)/pip_machdep.h

PROGS  = initfin stack export environ malloc file wait signal exit \
	mutex barrier core numa

all: $(PROGS)

%: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) $(CPPFLAGS) $(CCFLAGS) $(LDFLAGS) $< -o $@
	$(PATCHELF) $@

clean:
	rm -f $(PROGS) *.o *.E *~ .nfs*

test-file:
	( export LD_PRELOAD=$(PIPDIR)/preload/pip_preload.so; ./file )
