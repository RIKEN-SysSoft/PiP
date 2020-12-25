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
# $PIP_VERSION: Version 2.0.0$
#
# $Author: Atsushi Hori (R-CCS) mailto: ahori@riken.jp or ahori@me.com
# $

top_builddir = ..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk

PROGRAM = pip-exec
PROGRAMS = $(PROGRAM) printpipmode pip_unpie pipfc
PROGRAMS_TO_INSTALL = $(PROGRAMS) pipcc pip-mode pip-check pip-man pips 
EXTRA_PROGRAMS = pip-exec.to-install printpipmode.to-install

MAN1_SRCS = pip-exec.c pipcc.in pipfc.in pip-mode.in pip-check.in pip-man.in \
	pips-doxygen.c printpipmode.c

CPPFLAGS += $(PIP_CPPFLAGS) $(DBGFLAG)

debug:
	DBGFLAG=-DDEBUG $(MAKE) all

include $(top_srcdir)/build/rule.mk

all: $(EXTRA_PROGRAMS) FORCE

FORCE:
	chmod +x pipcc pipfc pip-check pip-mode pip-man pips
.PHONY: FORCE

pip-exec: pip-exec.c $(DEPS) Makefile
	$(CC) -g -O2 $(CPPFLAGS) $(PIP_CFLAGS_ROOT) $(PIP_LDFLAGS_ROOT) $< -o $@

pip-exec.to-install: pip-exec.c $(DEPS) Makefile
	$(CC) -g -O2 $(CPPFLAGS) $(PIP_CFLAGS_ROOT) $(PIP_LDFLAGS_ROOT_TO_INSTALL) $< -o $@

printpipmode: printpipmode.c $(DEPS) Makefile
	$(CC) -g -O2 $(CPPFLAGS) $(PIP_CFLAGS_BOTH) $(PIP_LDFLAGS_BOTH) $< -o $@

printpipmode.to-install: printpipmode.c $(DEPS) Makefile
	$(CC) -g -O2 $(CPPFLAGS) $(PIP_CFLAGS_BOTH) $(PIP_LDFLAGS_BOTH_TO_INSTALL) $< -o $@

install:
	$(MKDIR_P) $(DESTDIR)$(bindir)
	$(INSTALL_PROGRAM) pip-exec.to-install $(DESTDIR)$(bindir)/pip-exec
	$(INSTALL_PROGRAM) printpipmode.to-install $(DESTDIR)$(bindir)/printpipmode

pipfc: pipcc
	$(CP) pipcc pipfc

pip_unpie: pip_unpie.c
	$(CC) -g -O2 -Wall $< -o $@

post-clean-hook:
	$(RM) *.E *.log

post-veryclean-hook: post-clean-hook
	$(RM) pipcc pipfc pip-mode pip-check pip-man pips $(EXTRA_PROGRAMS)
	$(RM) .doxygen_*

post-distclean-hook: post-veryclean-hook
