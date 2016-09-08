
include ../../build/Make.common

CPPFLAGS = -I../ -I$(PIPINCDIR)
CCFLAGS += $(PIEFLAG)
LDFLAGS += -rdynamic $(PIPLIB)

SRCS    = a.c spawns.c spawnp.c

DEPINCS = ../test.h $(PIPINCDIR)/pip.h $(PIPINCDIR)/pip_machdep.h

TASKS = a b c
PROGS = $(TASKS) fort

all: $(PROGS)


b: a
	cp $< $@

c: a
	cp $< $@

%: %.c $(DEPINCS) $(PIPLIB) Makefile
	$(CC) $(CPPFLAGS) $(CCFLAGS) $(LDFLAGS) $< -o $@

%: %.f $(DEPINCS) $(PIPLIB) Makefile
	$(FC) $(CPPFLAGS) $(CCFLAGS) $(LDFLAGS) $< -o $@

clean:
	rm -f $(PROGS) *.o *.E *~ *.out .nfs*
