# $PIP_VERSION:$
# $PIP_license:$
# $RIKEN_copyright:$

SRCS  = main.c indirect.c

ASMS  = main.s indirect.s main-static.s indirect-static.s \
	main-none.s indirect-none.s  main-icc.s indirect-icc.s

OBJS  = main.o indirect.o
OBJS_ICC  = main-icc.o indirect-icc.o

PROGRAMS  = indirect-pie indirect-static indirect-none \
	indirect-icc-pie indirect-icc-static indirect-icc-none

CC = gcc
ICC = icc

OPTS = -O3

all: $(PROGRAMS) $(ASMS)

main.o: main.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -fpie -c $< -o $@

indirect.o: indirect.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -fpie -c $< -o $@

main-icc.o: main.c $(SRCS) $(DEPINCS) Makefile
	$(ICC) $(OPTS) -fpie -c $< -o $@

indirect-icc.o: indirect.c $(SRCS) $(DEPINCS) Makefile
	$(ICC) $(OPTS) -fpie -c $< -o $@


main.s: main.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -fpie -S $< -o $@

indirect.s: indirect.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -fpie -S $< -o $@

main-static.s: main.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -static -S $< -o $@

indirect-static.s: indirect.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -static -S $< -o $@

main-none.s: main.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -S $< -o $@

indirect-none.s: indirect.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -S $< -o $@

main-icc.s: main.c $(SRCS) $(DEPINCS) Makefile
	$(ICC) $(OPTS) -fpie -S $< -o $@

indirect-icc.s: indirect.c $(SRCS) $(DEPINCS) Makefile
	$(ICC) $(OPTS) -fpie -S $< -o $@


indirect-pie: $(OBJS)
	$(CC) $(OPTS) -pie $(OBJS) -o $@

indirect-static: $(SRCS)
	$(CC) $(OPTS) -static $(SRCS) -o $@

indirect-none: $(SRCS)
	$(CC) $(OPTS) -mcmodel=medium $(OBJS) -o $@

indirect-icc-pie: $(OBJS_ICC)
	$(ICC) $(OPTS) -pie $(OBJS_ICC) -o $@

indirect-icc-static: $(SRCS)
	$(ICC) $(OPTS) -static $(SRCS) -o $@

indirect-icc-none: $(SRCS)
	$(ICC) $(OPTS) -mcmodel=medium $(OBJS) -o $@

clean:
	rm -f $(PROGRAMS) *.o *.s *.S *~ *.log .nfs*
