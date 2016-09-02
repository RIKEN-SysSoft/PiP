
include ../build/Make.common

CPPFLAGS = -I$(PIPINCDIR)
CCFLAGS += $(PICFLAG)
LDFLAGS  = -pthread -ldl -shared -L$(GLIBCX)

LIBRARY  = libpip.so
SRCS     = pip.c pvas.c xpmem.c

OBJS	 = pip.o pvas.o xpmem.o

DEPINCS  = $(PIPINCDIR)/pip.h 			\
	   $(PIPINCDIR)/pip_internal.h 		\
	   $(PIPINCDIR)/pip_machdep.h 		\
	   $(PIPINCDIR)/pip_machdep_x86_64.h 	\
	   $(PIPINCDIR)/pvas.h 			\
	   $(PIPINCDIR)/pip_debug.h

.SUFFIXES: .c .o

all: $(LIBRARY)

%.o: %.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(CPPFLAGS) $(CCFLAGS) -c $< -o $@

$(LIBRARY): $(OBJS) Makefile
	$(LD) $(LDFLAGS) $(OBJS) -o $@

clean:
	rm -f $(LIBRARY) *.o *~ .nfs*
