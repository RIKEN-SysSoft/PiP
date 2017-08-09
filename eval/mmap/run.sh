#!/bin/sh

. ../eval.sh.inc

NTASKS=10

doeval() {
    for ITER in $ITER_NUM
    do
	echo -n "["$ITER"]" $1:$2 " "
	$MCEXEC ./$1 $3 $4
    done
    echo
}

csv_begin

for SZ in 4 8 16 32 64 128 256 512 1024
do
    for PROG in mmap-pip mmap-thread mmap-forkonly
    do
	if [ $PROG == mmap-pip ]
	then
	    for PIPMODE in $MODE_LIST
	    do
		export PIP_MODE=$PIPMODE;
		doeval $PROG $PIPMODE $NTASKS $SZ;
	    done
	else
	    doeval $PROG "" $NTASKS $SZ;
	fi
    done
    echo
done

csv_end
