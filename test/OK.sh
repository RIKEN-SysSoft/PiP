#!/bin/sh

dir=`dirname $0`
. $dir/exit_code.sh.inc

"$@";
extval=$?
if [ $extval -eq 0 ]; then
    exit $EXIT_PASS;
fi
exit $EXIT_FAIL;
