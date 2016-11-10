
include ../build/Make.common

PROGRAM = piprun

CPPFLAGS += -I$(PIPINCDIR) -DPROGRAM=\"$(PROGRAM)\"
LDFLAGS = -pthread -ldl $(PIPLIB)

DEPINCS = $(PIPINCDIR)/pip.h

$(PROGRAM): piprun.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) $(CPPFLAGS) $(CCFLAGS) $(LDFLAGS) $< -o $@
	$(PATCHELF) $@

clean:
	rm -f $(PROGRAM) *.o *.E *~ *.log .nfs*
