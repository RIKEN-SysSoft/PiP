#!/bin/sh
OMP_NUM_THREADS=`$MCEXEC ../util/ompnumthread`
NTASKS=`$MCEXEC ../util/dlmopen_count -p`
NPASS=`expr $NTASKS \- $OMP_NUM_THREADS`
tty > /dev/null 2>&1;
if [ $? -ne 0 ]
then
    QUIET='-q'
fi
exec ../loop.sh -t 10 $QUIET $MCEXEC $1 $2 $OMP_NUM_THREADS $NPASS 2>&1
