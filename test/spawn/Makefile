
include ../../build/Make.common

CPPFLAGS = -I../ -I$(PIPINCDIR)
CCFLAGS += $(PIEFLAG)
LDFLAGS += -rdynamic $(PIPLIB)

SRCS    = a.c spawns.c spawnp.c

DEPINCS = ../test.h $(PIPINCDIR)/pip.h $(PIPINCDIR)/pip_machdep.h

TASKS = b.out c.out
#PROGS = $(TASKS) spawns spawnp
PROGS = $(TASKS) spawns

all: $(PROGS)

b.out: a.out
	cp $< $@

c.out: a.out
	cp $< $@

a.out: a.c $(DEPINCS) $(PIPLIB) Makefile
	$(CC) $(CPPFLAGS) $(CCFLAGS) $(LDFLAGS) $< -o $@

%: %.c $(DEPINCS) $(PIPLIB) Makefile
	$(CC) $(CPPFLAGS) $(CCFLAGS) $(LDFLAGS) $< -o $@

clean:
	rm -f $(PROGS) *.o *.E *~ *.out .nfs*
