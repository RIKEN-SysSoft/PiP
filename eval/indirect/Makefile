# $PIP_VERSION:$
# $PIP_license:$
# $RIKEN_copyright:$

SRCS  = main.c indirect.c
ASMS  = main.s indirect.s main-static.s indirect-static.s main-none.s indirect-none.s
OBJS  = main.o indirect.o

PROGRAMS  = indirect indirect-static indirect-none

CC = gcc

all: $(PROGRAMS) $(ASMS)

main.o: main.c $(SRCS) $(DEPINCS) Makefile
	$(CC) -O2 -fpie -c $< -o $@

indirect.o: indirect.c $(SRCS) $(DEPINCS) Makefile
	$(CC) -O2 -fpie -c $< -o $@

main.s: main.c $(SRCS) $(DEPINCS) Makefile
	$(CC) -O2 -fpie -S $< -o $@

indirect.s: indirect.c $(SRCS) $(DEPINCS) Makefile
	$(CC) -O2 -fpie -S $< -o $@

main-static.s: main.c $(SRCS) $(DEPINCS) Makefile
	$(CC) -O2 -static -S $< -o $@

indirect-static.s: indirect.c $(SRCS) $(DEPINCS) Makefile
	$(CC) -O2 -static -S $< -o $@

main-none.s: main.c $(SRCS) $(DEPINCS) Makefile
	$(CC) -O2 -S $< -o $@

indirect-none.s: indirect.c $(SRCS) $(DEPINCS) Makefile
	$(CC) -O2 -S $< -o $@

indirect: $(OBJS)
	$(CC) -O2 -pie $(OBJS) -o $@

indirect-static: $(SRCS)
	$(CC) -O2 -static -mcmodel=medium $(OBJS) -o $@

indirect-none: $(SRCS)
	$(CC) -O2 -mcmodel=medium $(OBJS) -o $@

clean:
	rm -f $(PROGRAMS) *.o *.s *.S *~ *.log .nfs*
