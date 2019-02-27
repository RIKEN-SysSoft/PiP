#!/bin/sh

. ../test.sh.inc

LC_ALL=C; export LC_ALL

case $PIP_MODE in
'')
	PIP_MODE=process:preload; export PIP_MODE;;
process:preload)
	:;;
*)
	exit $EXIT_UNSUPPORTED;;
esac

# make ./pip_init.preload returns EPERM
unset LD_PRELOAD

if ./pip_init.preload 2>&1 | grep 'Operation not permitted' >/dev/null
then
  exit $EXIT_PASS
fi

exit $EXIT_FAIL
