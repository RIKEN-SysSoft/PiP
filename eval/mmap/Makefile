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

top_builddir = ../..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk
include $(srcdir)/../Eval.common

PIPINCFLAGS = -I$(srcdir)/..
PIPCFLAGS  = $(CFLAGS) $(PIEFLAG) $(PIPINCFLAGS)

DEPINCS = ../eval.h $(PIPINCDIR)/pip.h $(PIPINCDIR)/pip_machdep.h

SRCS  = mmap.c

PROGRAMS  = mmap-pip mmap-thread mmap-forkonly \
	mmap-pip-ac mmap-thread-ac mmap-forkonly-ac

PROGRAMS_TO_INSTALL = # nothing

include $(top_srcdir)/build/rule.mk

%-pip: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DPIP $(CPPFLAGS) $(PIPCFLAGS) $(PIPLDFLAGS) $< -o $@

%-thread: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DTHREAD $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -pthread $< -o $@

%-forkonly: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DFORK_ONLY $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $< -o $@

%-pip-ac: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DPIP -DACUM $(CPPFLAGS) $(PIPCFLAGS) $(PIPLDFLAGS) $< -o $@

%-thread-ac: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DTHREAD -DACUM $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -pthread $< -o $@

%-forkonly-ac: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DFORK_ONLY -DACUM $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $< -o $@

post-clean-hook:
	$(RM) *.E *.log $(PROGRAMS)

eval:
	( export LD_PRELOAD=$(build_topdir)/preload/pip_preload.so; numactl -C 1 ./mmap-pip 15 1024; numactl -C 1 ./mmap-thread 15 1024; numactl -C 1 ./mmap-forkonly 15 1024 )

eval-ac:
	( export LD_PRELOAD=$(build_topdir)/preload/pip_preload.so; numactl -C 1 ./mmap-pip-ac 15 1024; numactl -C 1 ./mmap-thread-ac 15 1024; numactl -C 1 ./mmap-forkonly-ac 15 1024 )
