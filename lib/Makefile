
include ../build/Make.common

CPPFLAGS = -I$(PIPINCDIR)
CCFLAGS += $(PICFLAG)
LDFLAGS  = -pthread -ldl -shared -L$(GLIBCX)

LIBRARY  = libpip.so
SRCS     = pip.c

DEPINCS  = $(PIPINCDIR)/pip.h 			\
	   $(PIPINCDIR)/pip_internal.h 		\
	   $(PIPINCDIR)/pip_machdep.h 		\
	   $(PIPINCDIR)/pip_machdep_x86_64.h 	\
	   $(PIPINCDIR)/pip_debug.h

.SUFFIXES: .c .o

all: $(LIBRARY)

$(LIBRARY): $(SRCS) $(DEPINCS) Makefile
	$(CC) $(CPPFLAGS) $(CCFLAGS) $(LDFLAGS) $(SRCS) -o $(LIBRARY)

clean:
	rm -f $(LIBRARY) *.o *~ .nfs*
