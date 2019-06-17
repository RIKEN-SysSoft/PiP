#!/bin/sh
EXIT_UNTESTED=5		# not tested, this test hasn't been written yet
if [[ $PIP_MODE != 'pthread' ]]; then
    exec $MCEXEC $1 $2 $OMP_NUM_THREADS $OMP_NUM_THREADS 2>&1;
else
    exit $EXIT_UNTESTED;
fi
