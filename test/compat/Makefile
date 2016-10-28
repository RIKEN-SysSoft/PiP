
include ../../build/Make.common
include ../Test.common

SRCS  = gethostname.c gethostbyname.c initfini.c iftask.c gettimeofday.c \
	ctype.c

PROGS = gethostname gethostbyname dlopen initfini iftask gettimeofday \
	ctype

all: $(PROGS)

clean:
	rm -f $(PROGS) *.o *.E *~ *.log .nfs*
