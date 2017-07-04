#!/bin/sh

. ../eval.sh.inc

PROGS="./mmap-pip ./mmap-thread ./mmap-forkonly"

doeval() {
    if [ $1 == mmap-pip ]
    then
	for PIPMODE in $MODE_LIST
	do
	    export PIP_MODE=$PIPMODE;
	    for ITER in 1 2 3 4 5 6 7 8 9 10 11 12
	    do
		echo -n "["$ITER"]" $1:$PIPMODE $2 [Tasks] $3 "[MB]"
		$1 $2 $3
	    done
	    echo
	done
    else
	for ITER in 1 2 3 4 5 6 7 8 9 10 11 12
	do
	    echo -n "["$ITER"]" $1 $2 [Tasks] $3 "[MB]"
	    ./$1 $2 $3
	done
	echo
    fi
}

csv_begin

for NTASKS in 10 20 40 80 160
do
    for PROG in ${PROGS}
    do
	doeval ${PROG} ${NTASKS} 128
    done
done

csv_end
