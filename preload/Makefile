
include ../build/Make.common

CPPFLAGS = -I$(PIPINCDIR)
CCFLAGS += $(PICFLAG)
LDFLAGS  = -shared

LIBRARY  = pip_preload.so
SRCS     = pip_preload.c

DEPINCS  = $(PIPINCDIR)/pip_debug.h

.SUFFIXES: .c .o

all: $(LIBRARY)

$(LIBRARY): $(SRCS) $(DEPINCS) Makefile
	$(CC) $(CPPFLAGS) $(CCFLAGS) $(LDFLAGS) $(SRCS) -o $(LIBRARY)

clean:
	rm -f $(LIBRARY) *.o *~
