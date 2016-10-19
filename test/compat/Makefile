
include ../../build/Make.common
include ../Test.common

SRCS  = gethostname.c gethostbyname.c initfini.c iftask.c gettimeofday.c

PROGS = gethostname gethostbyname dlopen initfini iftask gettimeofday

all: $(PROGS)

clean:
	rm -f $(PROGS) *.o *.E *~ *.log .nfs*
