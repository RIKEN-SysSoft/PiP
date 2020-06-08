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

PROGRAMS = pip-exec abc def ghi xyz hello

PROGRAMS_TO_INSTALL = # nothing

CPPFLAGS += -I$(PIPINCDIR)
LDLIBS += $(PIPLDLIB) -ldl

DEPLIBS = $(PIPLDLIB)
DEPINCS = $(PIPINCDIR)/pip.h

PIPCC = ../scripts/pipcc

include $(top_srcdir)/build/rule.mk
include $(srcdir)/../Test.common

pip-exec : ../../bin/pip-exec
	rm -f pip-exec
	ln -s ../../bin/pip-exec

abc: abc.c $(DEPINCS) Makefile
	$(PIPCC) -DMSG=\"abc\" $< -o $@

def: abc.c $(DEPINCS) Makefile
	$(PIPCC) -DMSG=\"def\" $< -o $@

ghi: abc.c $(DEPINCS) Makefile
	$(PIPCC) -DMSG=\"ghi\" $< -o $@

xyz: abc.c $(DEPINCS) Makefile
	$(PIPCC) -DMSG=\"xyz\" $< -o $@

hello.c: ../scripts/hello.c
	cp -f ../scripts/hello.c .

hello: hello.c
	$(PIPCC) $< -o $@

test:
	../test.sh -A test.list

install_test_prog = $(install_test_dir)/prog

install-test:
	$(MKDIR_P) $(install_test_prog)
	$(INSTALL) -C -m 644 Makefile abc.c hello.c test.list $(install_test_prog)
	ln -f -s $(prefix)/bin/pip-exec $(install_test_prog)

install-test-all: abc def ghi xyz hello
