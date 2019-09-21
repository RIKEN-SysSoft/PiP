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
include $(srcdir)/../Test.common

SRCS = abc.c

PROGRAMS_TO_INSTALL =
PROGRAMS = abc def ghi xyz

CFLAGS += -pthread
CPPFLAGS += -I$(PIPINCDIR)
LDLIBS += $(PIPLDLIB) -ldl

DEPINCS = $(PIPINCDIR)/pip.h $(PIPINCDIR)/pip_util.h

include $(top_srcdir)/build/rule.mk

abc: abc.c $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DMSG=\"abc\" -I$(PIPINCDIR) $(LDFLAGS) -O2 \
	-fpie -pie -rdynamic $< -o $@

def: abc.c $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DMSG=\"def\" -I$(PIPINCDIR) $(LDFLAGS) -O2 \
	-fpie -pie -rdynamic $< -o $@

ghi: abc.c $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DMSG=\"ghi\" -I$(PIPINCDIR) $(LDFLAGS) -O2 \
	-fpie -pie -rdynamic $< -o $@

xyz: abc.c $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DMSG=\"xyz\" -DPIP=0 -I$(PIPINCDIR) $(PIPLDFLAGS) -O2 \
	-fpie -pie -rdynamic $< -o $@
