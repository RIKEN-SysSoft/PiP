
include ../../build/Make.common
include ../Test.common

SRCS  = gethostname.c gethostbyname.c initfini.c iftask.c gettimeofday.c \
	ctype.c exit.c brk.c pf.c

PROGS = gethostname gethostbyname dlopen initfini iftask gettimeofday \
	ctype exit brk pf

all: $(PROGS)

clean:
	rm -f $(PROGS) *.o *.E *~ *.log .nfs*
