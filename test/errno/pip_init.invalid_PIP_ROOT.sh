#!/bin/sh

. ../test.sh.inc

LC_ALL=C; export LC_ALL

if PIP_ROOT=0 ./pip_init 2>&1 | grep 'Operation not permitted' >/dev/null
then
  exit $EXIT_PASS
fi

exit $EXIT_FAIL
