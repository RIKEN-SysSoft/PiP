#!/bin/sh
EXIT_UNTESTED=5		# not tested, this test hasn't been written yet
NPASS=`expr $NTASKS \- $OMP_NUM_THREADS`
if [[ $PIP_MODE != 'pthread' ]]; then
    exec $MCEXEC $1 $2 $OMP_NUM_THREADS $NPASS 2>&1;
else
    exit $EXIT_UNTESTED;
fi
