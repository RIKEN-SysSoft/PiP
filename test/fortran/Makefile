
include ../../build/Make.common
include ../Test.common

SRCS    = a.c spawns.c spawnp.c

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
