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

DEPINCS = $(srcdir)/../eval.h $(PIPINCDIR)/pip.h

SRCS  = spawn.c

PROGRAMS  = foo spawn-pip spawn-forkexec spawn-vforkexec spawn-posix spawn-thread

PROGRAMS_TO_INSTALL = # nothing

include $(top_srcdir)/build/rule.mk

foo: foo.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -fpie -pie -rdynamic -pthread $< -o $@

%-pip: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DPIP $(CPPFLAGS) $(CFLAGS) $(PIPLDFLAGS) $< -o $@

%-forkexec: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DFORK_EXEC $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $< -o $@

%-vforkexec: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DVFORK_EXEC $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $< -o $@

%-posix: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DPOSIX_SPAWN $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $< -o $@

%-thread: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DTHREAD $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -pthread $< -o $@

post-clean-hook:
	$(RM) *.E *.log

eval:
	numactl -C 0 ./spawn-pip 50
	numactl -C 0 ./spawn-forkexec 50
	numactl -C 0 ./spawn-vforkexec 50
	numactl -C 0 ./spawn-posix 50
	numactl -C 0 ./spawn-thread 50
