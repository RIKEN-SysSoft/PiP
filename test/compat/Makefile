
include ../../build/Make.common

CPPFLAGS = -I../ -I$(PIPINCDIR)
CCFLAGS += $(PIEFLAG)
LDFLAGS += -rdynamic $(PIPLIB)

SRCS  = gethostname.c gethostbyname.c initfini.c iftask.c gettimeofday.c

DEPINCS = ../test.h $(PIPINCDIR)/pip.h $(PIPINCDIR)/pip_machdep.h

PROGS  = gethostname gethostbyname dlopen initfini iftask gettimeofday

all: $(PROGS)

%: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) $(CPPFLAGS) $(CCFLAGS) $(LDFLAGS) $< -o $@
	$(PATCHELF) $@

clean:
	rm -f $(PROGS) *.o *.E *~ *.log .nfs*
