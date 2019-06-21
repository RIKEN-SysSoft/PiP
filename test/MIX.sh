#!/bin/sh
pushd ..
. ./test.sh.inc
popd
if [[ $PIP_MODE != 'pthread' ]]; then
    NPASS=`expr $OMP_NUM_THREADS \* 2`;
    exec $MCEXEC $1 $2 $OMP_NUM_THREADS $NPASS 2>&1;
else
    exit $EXIT_UNTESTED;
fi
