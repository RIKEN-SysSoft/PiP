
include ../../build/Make.common
include ../Test.common

OMPFLAG = -fopenmp

CCFLAGS += $(OMPFLAG)
LDFLAGS += $(OMPFLAG)

SRCS  = openmp.c

PROGS  = openmp

all: $(PROGS)

%: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) $(CPPFLAGS) $(CCFLAGS) $(LDFLAGS) $< -o $@

##	$(PATCHELF) $@ <<< if you want to do this, we need libgomp.so
##			   compiled and linked with our ld-linux.so
