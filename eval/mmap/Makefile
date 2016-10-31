
include ../../build/Make.common
include ../Eval.common

SRCS  = mmap.c

PROGS  = mmap-pip mmap-thread mmap-forkonly

all: $(PROGS)

%-pip: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DPIP $(CPPFLAGS) $(CCFLAGS) $(LDFLAGS) $< -o $@
	$(PATCHELF) $@

%-thread: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DTHREAD $(CPPFLAGS) $(CCFLAGS) $(LDFLAGS) $< -o $@

%-forkonly: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DFORK_ONLY $(CPPFLAGS) $(CCFLAGS) $(LDFLAGS) $< -o $@

clean:
	rm -f $(PROGS) *.o *.E *~ *.log .nfs*

eval:
	( export LD_PRELOAD=$(PIPDIR)/preload/pip_preload.so; numactl -C 0 ./mmap-pip 15; numactl -C 0 ./mmap-thread 15; numactl -C 0 ./mmap-forkonly 15 )
