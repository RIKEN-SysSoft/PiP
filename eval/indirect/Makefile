# $PIP_VERSION: Version 1.0$
# $PIP_license: <Simplified BSD License>
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
# 
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the 
#    documentation and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# 
# The views and conclusions contained in the software and documentation
# are those of the authors and should not be interpreted as representing
# official policies, either expressed or implied, of the PiP project.$
# $RIKEN_copyright: 2018 Riken Center for Computational Sceience, 
# 	  System Software Devlopment Team. All rights researved$

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
