#!/bin/sh

. ../test.sh.inc

ntasks=`getconf _NPROCESSORS_ONLN`
if [ $ntasks -gt 10 ]; then
  ntasks=10
fi

$MCEXEC ./pf 2>&1 | test_msg_count 'done' `expr $ntasks + 1`
