#!/bin/sh
pushd ..
. ./test.sh.inc
popd
if [[ $PIP_MODE != 'pthread' ]]; then
    NPASS=`expr $NTASKS / 2`;
    exec $MCEXEC $1 $2 $NPASS $NPASS 2>&1;
else
    exit $EXIT_UNTESTED;
fi
