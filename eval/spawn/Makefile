
include ../../build/Make.common
include ../Eval.common

PIPLDFLAGS += $(PIPLIB) $(LDFLAGS)
FOOLDFLAGS += -rdynamic $(LDFLAGS)

DEPINCS = ../eval.h $(PIPINCDIR)/pip.h

SRCS  = spawn.c

PROGS  = foo spawn-pip spawn-forkexec spawn-thread

all: $(PROGS)

foo: foo.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) $(CPPFLAGS) $(CCFLAGS) $(FOOLDFLAGS) -Bdynamic $< -o $@

%-pip: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DPIP $(CPPFLAGS) $(CCFLAGS) $(PIPLDFLAGS) $< -o $@
	$(PATCHELF) $@

%-forkexec: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DFORK_EXEC $(CPPFLAGS) $(CCFLAGS) $(LDFLAGS) $< -o $@

%-thread: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DTHREAD $(CPPFLAGS) $(CCFLAGS) $(LDFLAGS) $< -o $@

clean:
	rm -f $(PROGS) *.o *.E *~ *.log .nfs*

eval:
	( export LD_PRELOAD=$(PIPDIR)/preload/pip_preload.so; \
	numactl -C 0 ./spawn-pip 50; \
	numactl -C 0 ./spawn-forkexec 50; \
	numactl -C 0 ./spawn-thread 50 )
