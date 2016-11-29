# $PIP_VERSION:$
# $PIP_license:$
# $RIKEN_copyright:$

SRCS  = main.c indirect.c

ASMS  = main.s indirect.s main-static.s indirect-static.s \
	main-none.s indirect-none.s main-lto.s indirect-lto.s \
	main-static-lto.s indirect-static-lto.s \
	main-medium-static.s indirect-medium-static.s \
	main-medium.s indirect-medium.s \
	main-large-static.s indirect-large-static.s \
	main-large.s indirect-large.s

OBJS  = main.o indirect.o
OBJS_LTO  = main-lto.o indirect-lto.o
OBJS_MED  = main-medium.o indirect-medium.o
OBJS_LRG  = main-large.o indirect-large.o
OBJS_32  = main-32.o indirect-32.o

PROGRAMS  = indirect indirect-static indirect-none \
	indirect-lto indirect-static-lto indirect \
	indirect-medium-static indirect-medium \
	indirect-large-static indirect-large

CC = gcc

OPTS = -O3

all: $(PROGRAMS) $(ASMS)

main.o: main.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -fpie -c $< -o $@

indirect.o: indirect.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -fpie -c $< -o $@

main-32.o: main.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -fpie -m32 -c $< -o $@

indirect-32.o: indirect.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -fpie -m32 -c $< -o $@

main-lto.o: main.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -flto -fpie -c $< -o $@

indirect-lto.o: indirect.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -flto -fpie -c $< -o $@

main-medium.o: main.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -mcmodel=medium -fpie -c $< -o $@

indirect-medium.o: indirect.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -mcmodel=medium -fpie -c $< -o $@

main-large.o: main.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -mcmodel=large -fpie -c $< -o $@

indirect-large.o: indirect.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -mcmodel=large -fpie -c $< -o $@


main.s: main.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -fpie -S $< -o $@

indirect.s: indirect.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -fpie -S $< -o $@

main-static.s: main.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -static -S $< -o $@

indirect-static.s: indirect.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -static -S $< -o $@

main-32.s: main.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -fpie -m32 -S $< -o $@

indirect-32.s: indirect.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -fpie -m32 -S $< -o $@

main-32-static.s: main.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -static -m32 -S $< -o $@

indirect-32-static.s: indirect.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -static -m32 -S $< -o $@

main-none.s: main.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -S $< -o $@

indirect-none.s: indirect.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -S $< -o $@

main-static-lto.s: main.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -flto -static -S $< -o $@

indirect-static-lto.s: indirect.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -flto -static -S $< -o $@

main-lto.s: main.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -flto -fPIC -S $< -o $@

indirect-lto.s: indirect.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -flto -fPIC -S $< -o $@

main-medium-static.s: main.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -static -mcmodel=medium -S $< -o $@

indirect-medium-static.s: indirect.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -static -mcmodel=medium -S $< -o $@

main-large-static.s: main.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -static -mcmodel=large -S $< -o $@

indirect-large-static.s: indirect.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -static -mcmodel=large -S $< -o $@

main-medium.s: main.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -fPIC -mcmodel=medium -S $< -o $@

indirect-medium.s: indirect.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -fPIC -mcmodel=medium -S $< -o $@

main-large.s: main.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -fPIC -mcmodel=large -S $< -o $@

indirect-large.s: indirect.c $(SRCS) $(DEPINCS) Makefile
	$(CC) $(OPTS) -fPIC -mcmodel=large -S $< -o $@


indirect: $(OBJS)
	$(CC) $(OPTS) -pie $(OBJS) -o $@

indirect-static: $(SRCS)
	$(CC) $(OPTS) -static -mcmodel=medium $(SRCS) -o $@

indirect-none: $(SRCS)
	$(CC) $(OPTS) -mcmodel=medium $(OBJS) -o $@

indirect-lto: $(OBJS_LTO)
	$(CC) $(OPTS) -flto -pie $(OBJS_LTO) -o $@

indirect-static-lto: $(SRCS)
	$(CC) $(OPTS) -flto -static -mcmodel=medium $(SRCS) -o $@

indirect-medium: $(OBJS_MED)
	$(CC) $(OPTS) -pie $(OBJS_MED) -o $@

indirect-medium-static: $(SRCS)
	$(CC) $(OPTS) -static -mcmodel=medium $(SRCS) -o $@

indirect-large: $(OBJS_LRG)
	$(CC) $(OPTS) -pie $(OBJS_LRG) -o $@

indirect-large-static: $(SRCS)
	$(CC) $(OPTS) -static -mcmodel=medium $(SRCS) -o $@

indirect-32-static: $(SRCS)
	$(CC) $(OPTS) -static $(SRCS) -m32 -o $@

indirect-32: $(OBJS_32)
	$(CC) $(OPTS) -pie $(OBJS_32) -m32 -o $@

clean:
	rm -f $(PROGRAMS) *.o *.s *.S *~ *.log .nfs*
