#!/bin/sh

. ../eval.sh.inc

NTASKS=50

doeval() {
    for ITER in 1 2 3 4 5 6 7 8 9 10 11 12
    do
	echo -n "["$ITER"]" $1:$2 " "
#	NUMACTL 3 ./$1 $4 $3
	NUMACTL 3 ./$1 $3
    done
    echo
}

csv_begin

#for TOUCH in 0 1 2 4 8 16 32 64 128 256 512 1024 2048 4096
for TOUCH in 1 2 4 8 16 32
do
#    for PROG in ctxsw-pip ctxsw-thread ctxsw-forkonly
    for PROG in futex-pip futex-thread futex-forkonly
    do
#	if [ $PROG == ctxsw-pip ]
	if [ $PROG == futex-pip ]
	then
	    for PIPMODE in $MODE_LIST
	    do
		doeval $PROG $PIPMODE $TOUCH $NTASKS
	    done
	else
	    doeval $PROG "" $TOUCH $NTASKS;
	fi
    done
    echo
done

csv_end
