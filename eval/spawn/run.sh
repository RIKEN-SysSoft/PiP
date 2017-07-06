#!/bin/sh

. ../eval.sh.inc

csv_begin

for NTASKS in 1 2 4 8 16 32 64 128
do
    if [ $NTASKS -gt $NTMAX ]
    then
	 break
    fi
    for PROG in spawn-pip spawn-forkexec spawn-vforkexec spawn-posix spawn-thread
    do
	if [ $PROG == spawn-pip ]
	then
	    for PIPMODE in $MODE_LIST
	    do
		export PIP_MODE=$PIPMODE
		for ITER in $ITER_NUM
		do
		    NUMACTL 3 ./$PROG $NTASKS $NTASKS
		    #./$PROG $NTASKS $NTASKS
		done
		unset PIP_MODE
		echo
	    done
	else
	    for ITER in $ITER_NUM
	    do
		NUMACTL 3 ./$PROG $NTASKS $NTASKS
		#./$PROG $NTASKS $NTASKS
	    done
	    echo
	fi
    done
    echo
done

csv_end
