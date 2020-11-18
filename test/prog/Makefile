# $PIP_VERSION: Version 2.0.0$
# $PIP_license: <Simplified BSD License>
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
#     Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
# 
#     Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.
# $
# $RIKEN_copyright: Riken Center for Computational Sceience (R-CCS),
# System Software Development Team, 2016-2020
# $

top_builddir = ../..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk

PROGRAMS = pip-exec abc def ghi xyz hello

PROGRAMS_TO_INSTALL = # nothing

PIPCC = ../scripts/pipcc

include $(top_srcdir)/build/rule.mk
include $(srcdir)/../Test.common

pip-exec : ../../bin/pip-exec
	$(RM) pip-exec
	ln -s ../../bin/pip-exec

abc: abc.c $(DEPINCS) Makefile
	$(PIPCC) $< -o $@

def: abc.c $(DEPINCS) Makefile
	$(PIPCC) $< -o $@

ghi: abc.c $(DEPINCS) Makefile
	$(PIPCC) $< -o $@

xyz: abc.c $(DEPINCS) Makefile
	$(PIPCC) $< -o $@

hello.c: ../scripts/hello.c
	cp -f ../scripts/hello.c .

hello: hello.c
	$(PIPCC) $< -o $@

test:
	../test.sh -A test.list

install_test_prog_dir = $(install_test_dir)/prog

check-installed:
	$(MKDIR_P) $(install_test_prog_dir)
	$(INSTALL) -C -m 644 Makefile abc.c hello.c test.list $(install_test_prog_dir)
	ln -f -s $(prefix)/bin/pip-exec $(install_test_prog_dir)
