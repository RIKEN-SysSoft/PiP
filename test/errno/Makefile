# $RIKEN_copyright: Riken Center for Computational Sceience,
# System Software Development Team, 2016, 2017, 2018, 2019$
# $PIP_VERSION: Version 1.0.0$
# $PIP_license$

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

