#!/bin/sh

dir=`dirname $0`
. $dir/../exit_code.sh.inc

../NG.sh ../util/timer 1 ./inf
rv=$?;
if [ $rv -ne 0 ]; then
    exit $EXIT_FAIL;
fi

../NG.sh ../util/timer 1 ../util/pip_task 1 ./inf
rv=$?;
if [ $rv -ne 0 ]; then
    exit $EXIT_FAIL;
fi

../NG.sh ../util/timer 1 ../util/pip_task 8 ./inf
rv=$?;
if [ $rv -ne 0 ]; then
    exit $EXIT_FAIL;
fi

../NG.sh ../util/timer 1 ../util/pip_blt 1 1 ./inf
rv=$?;
if [ $rv -ne 0 ]; then
    exit $EXIT_FAIL;
fi

../NG.sh ../util/timer 1 ../util/pip_blt 8 8 ./inf
rv=$?;
if [ $rv -ne 0 ]; then
    exit $EXIT_FAIL;
fi

../NG.sh ../util/timer 1 ../util/pip_blt 8 24 ./inf
rv=$?;
if [ $rv -ne 0 ]; then
    exit $EXIT_FAIL;
fi

exit $EXIT_PASS;
