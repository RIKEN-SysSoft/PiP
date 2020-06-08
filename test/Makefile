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

top_builddir = ..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk

PROGRAMS_TO_INSTALL = # nothing

include $(top_srcdir)/build/rule.mk
include ./Test.common

install-test-all:
	make -C util
	make -C prog install-test-all
	make -C basics
	make -C compat
	make -C pthread
	make -C openmp
	make -C cxx
	make -C fortran
	make -C issues
	make -C blt

all :
	make -C scripts
	make -C prog
	make -C util
	make -C prog
	make -C basics
	make -C compat
	make -C pthread
	make -C openmp
	make -C cxx
	make -C fortran
	make -C issues
	make -C blt

clean:
	rm -f $(OBJS)

.PHONY: test
test:
	./test.sh -A test.list

testclean:
	make -C scripts testclean
	make -C util testclean
	make -C prog testclean
	make -C basics testclean
	make -C compat testclean
	make -C pthread testclean
	make -C openmp testclean
	make -C	cxx testclean
	make -C fortran testclean
	make -C issues testclean
	make -C blt testclean

TESTLISTS = bin.list sample.list test.list
SCRIPTS   = test.sh loop.sh NG.sh OK.sh skip.sh exit_code.sh.inc
HEADERS   = test.h

install-test :
	$(INSTALL) -C -m 744 Makefile Test.common $(TESTLISTS) $(HEADERS) $(install_test_dir)
	$(INSTALL) -C -m 755 $(SCRIPTS) $(install_test_dir)
	make -C scripts install-test
	make -C util install-test
	make -C prog install-test
	make -C basics install-test
	make -C compat install-test
	make -C pthread install-test
	make -C openmp install-test
	make -C cxx install-test
	make -C fortran install-test
	make -C issues install-test
	make -C blt install-test
