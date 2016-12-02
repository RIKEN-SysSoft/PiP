
include ../../build/Make.common
include ../Eval.common

PIPINCFLAGS = -I..
PIPCCFLAGS  = $(CCFLAGS) $(PIEFLAG) $(PIPINCFLAGS)
PIPLDFLAGS  = $(LDFLAGS) $(PIPLIB) -rdynamic

DEPINCS = ../eval.h $(PIPINCDIR)/pip.h $(PIPINCDIR)/pip_machdep.h

SRCS  = ctxsw.c

PROGS  = ctxsw-pip ctxsw-thread ctxsw-forkonly

all: $(PROGS)

%-pip: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DPIP $(CPPFLAGS) $(PIPCCFLAGS) $(PIPLDFLAGS) $< -o $@
	$(PATCHELF) $@

%-thread: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DTHREAD $(CPPFLAGS) $(CCFLAGS) $(LDFLAGS) $< -o $@

%-forkonly: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DFORK_ONLY $(CPPFLAGS) $(CCFLAGS) $(LDFLAGS) $< -o $@

clean:
	rm -f $(PROGS) *.o *.E *~ *.log .nfs*

eval:
	( export LD_PRELOAD=$(PIPDIR)/preload/pip_preload.so; numactl -C 0 ./ctxsw-pip 15 1024; numactl -C 0 ./ctxsw-thread 15 1024; numactl -C 0 ./ctxsw-forkonly 15 1024 )
