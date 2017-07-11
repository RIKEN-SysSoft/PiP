#!/bin/sh

. ../eval.sh.inc

PROGS="stencil_pip stencil_mpi"
SIZE=3200
ENERGY=1
NITERS=1000
PIPRUN=`pwd`/../../bin/piprun

doeval() {
    if [ $1 == stencil_pip ]
    then
	for PIPMODE in $MODE_LIST
	do
	    export PIP_MODE=$PIPMODE;
	    for ITER in $ITER_NUM
	    do
		echo -n "["$ITER"]" $1:$PIPMODE $2
		$PIPRUN -n $2 ./$1 $SIZE $ENERGY $NITERS
	    done
	    echo
	done
    else
	for ITER in $ITER_NUM
	do
	    echo -n "["$ITER"]" $1 $2
	    mpirun -np $2 ./$1 $SIZE $ENERGY $NITERS
	done
	echo
    fi
}

csv_begin

for NTASKS in 1 2 4 8 16
do
    if [ $NTASKS -gt $NTMAX ]
    then
	 break
    fi
    for PROG in ${PROGS}
    do
	doeval ${PROG} ${NTASKS}
    done
done

csv_end
