
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
