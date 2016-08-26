
include ../../build/Make.common

OMPFLAG = -fopenmp

CPPFLAGS = -I../ -I$(PIPINCDIR)
CCFLAGS += $(OMPFLAG) $(PIEFLAG)
LDFLAGS += -rdynamic $(CCFLAGS) $(PIPLIB) $(OMPFLAG)

SRCS  = openmp.c

DEPINCS = ../test.h $(PIPINCDIR)/pip.h $(PIPINCDIR)/pip_machdep.h

PROGS  = openmp

all: $(PROGS)

%: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) $(CPPFLAGS) $(CCFLAGS) $(LDFLAGS) $< -o $@

##	$(PATCHELF) $@ <<< if you want to do this, we need libgomp.so
##			   compiled and linked with our ld-linux.so


clean:
	rm -f $(PROGS) *.o *.E *~ .nfs*
