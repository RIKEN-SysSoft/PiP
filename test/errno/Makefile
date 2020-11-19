# $RIKEN_copyright: Riken Center for Computational Sceience (R-CCS),
# System Software Development Team, 2016-2020
# $
# $PIP_VERSION: Version 1.2.0$
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

top_builddir = ../..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk
include $(srcdir)/../Test.common

SRCS  = pip_init.c pip_fin.c pip_spawn.c pip_get_ntasks.c \
	pip_get_mode.c pip_get_mode_str.c

PROGRAMS_PIP_INIT = \
	pip_init.preload \
	pip_init.twice \
	pip_init.ntask_is_zero \
	pip_init.ntask_too_big \
	pip_init.invalid_opts \
	pip_init.both_pthread_process \
	pip_init.both_preload_clone \
	pip_init.pip_task_unset

PROGRAMS  = pip_init $(PROGRAMS_PIP_INIT) pip_fin pip_spawn pip_get_ntasks \
	pip_get_mode pip_get_mode_str

PROGRAMS_TO_INSTALL = # nothing

include $(top_srcdir)/build/rule.mk

$(PROGRAMS_PIP_INIT): pip_init
	rm -f $@; ln pip_init $@

post-clean-hook:
	$(RM) *.E *.log

