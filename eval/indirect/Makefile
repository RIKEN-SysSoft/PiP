# $PIP_VERSION:$
# $PIP_license:$
# $RIKEN_copyright:$

SRCS  = main.c indirect.c

ASMS  = main.s indirect.s main-static.s indirect-static.s \
	main-none.s indirect-none.s  main-icc.s indirect-icc.s

OBJS  = main.o indirect.o libind.so
#OBJS_ICC  = main-icc.o indirect-icc.o

PROGRAMS  = indirect-pie indirect-static indirect-none
#PROGRAMS  = indirect-pie indirect-static indirect-none \
#	indirect-icc-pie indirect-icc-static indirect-icc-none

CC = gcc
#ICC = icc
ICC = echo

OPTS = -O3 -g

all: $(PROGRAMS) $(ASMS)

main.o: main.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -fpie -c $< -o $@

libind.so: indirect.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -fpie -c -shared $< -o libind.so

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


indirect-pie: main.o libind.so
	$(CC) $(OPTS) main.o -pie -L. -lind -o $@ -Wl,--no-relax

indirect-static: $(SRCS)
	$(CC) $(OPTS) -static $(SRCS) -o $@

indirect-none: $(SRCS)
	$(CC) $(OPTS) $(SRCS) -o $@

indirect-icc-pie: $(OBJS_ICC)
	$(ICC) $(OPTS) -pie $(OBJS_ICC) -o $@

indirect-icc-static: $(SRCS)
	$(ICC) $(OPTS) -static $(SRCS) -o $@

indirect-icc-none: $(SRCS)
	$(ICC) $(OPTS) -mcmodel=medium $(SRCS) -o $@

clean:
	rm -f $(PROGRAMS) *.o *.s *.S *.so *~ *.log .nfs*
