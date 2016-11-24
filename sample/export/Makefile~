
include ../../build/Make.common

PIPCCFLAGS  = -I$(PIPINCDIR) $(CCFLAGS) $(PIEFLAG) $(PIPINCFLAGS)

DEPINCS = $(PIPINCDIR)/pip.h

SRCS  = hello.c

PROGS  = hello

all: $(PROGS)

%: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) $(PIPCCFLAGS) $(CCFLAGS) $(PIPLDFLAGS) $< -o $@

clean:
	rm -f $(PROGS) *.o *.E *~ *.log .nfs*
