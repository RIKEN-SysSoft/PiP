
include ../../build/Make.common
include ../Eval.common

PIPINCFLAGS = -I..
PIPCCFLAGS  = $(CCFLAGS) $(PIEFLAG) $(PIPINCFLAGS)

DEPINCS = ../eval.h $(PIPINCDIR)/pip.h $(PIPINCDIR)/pip_machdep.h

SRCS  = mmap.c

PROGS  = mmap-pip mmap-thread mmap-forkonly \
	mmap-pip-ac mmap-thread-ac mmap-forkonly-ac

all: $(PROGS)

%-pip: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DPIP $(CPPFLAGS) $(PIPCCFLAGS) $(PIPLDFLAGS) $< -o $@

%-thread: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DTHREAD $(CPPFLAGS) $(CCFLAGS) $(LDFLAGS) -pthread $< -o $@

%-forkonly: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DFORK_ONLY $(CPPFLAGS) $(CCFLAGS) $(LDFLAGS) $< -o $@

%-pip-ac: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DPIP -DACUM $(CPPFLAGS) $(PIPCCFLAGS) $(PIPLDFLAGS) $< -o $@

%-thread-ac: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DTHREAD -DACUM $(CPPFLAGS) $(CCFLAGS) $(LDFLAGS) -pthread $< -o $@

%-forkonly-ac: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DFORK_ONLY -DACUM $(CPPFLAGS) $(CCFLAGS) $(LDFLAGS) $< -o $@

clean:
	rm -f $(PROGS) *.o *.E *~ *.log .nfs*

eval:
	( export LD_PRELOAD=$(PIPDIR)/preload/pip_preload.so; numactl -C 0 ./mmap-pip 15 1024; numactl -C 0 ./mmap-thread 15 1024; numactl -C 0 ./mmap-forkonly 15 1024 )

eval-ac:
	( export LD_PRELOAD=$(PIPDIR)/preload/pip_preload.so; numactl -C 0 ./mmap-pip-ac 15 1024; numactl -C 0 ./mmap-thread-ac 15 1024; numactl -C 0 ./mmap-forkonly-ac 15 1024 )
