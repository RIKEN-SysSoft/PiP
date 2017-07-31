#!/bin/sh

. ../eval.sh.inc

PROGS="stencil_pip stencil_mpi"
FPROGS="stencil_pip_fast stencil_mpi_fast"
#SIZE=4096
SIZE=8192
ENERGY=1
NITERS=1000
FAST=1
PIPRUN=`pwd`/../../bin/piprun
MODES="process"

MPIBIND="-binding map=0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31"

doperf() {
#    for ITER in 1 2 3 4 5 6 7 8 9 10 11 12
    for ITER in 1 2 3 4
    do
	if [ $1 == stencil_pip_fast ]
	then
	    PIPMODE=process
	    export PIP_MODE=$PIPMODE;
	    echo -n "["$ITER"]" $1:$PIPMODE $2 "(" $3 ") "
	    $PIPRUN -b -n $2 ./$1 $SIZE $ENERGY $3
	else
	    echo -n "["$ITER"]" $1 $2 "(" $3 ") "
	    mpirun -np $2 $MPIBIND ./$1 $SIZE $ENERGY $3
	fi
    done
}

csv_begin perf

for LCOUNT in 2 30 100 300 1000 3000 10000
do
    for NTASKS in 4 8 16 32 64
    do
	if [ $NTASKS -gt $NTMAX ]
	then
	    break
	fi
	if [ $NTASKS -gt $CORENUM ]
	then
	    break
	fi
	for PROG in ${FPROGS}
	do
	    doperf ${PROG} ${NTASKS} ${LCOUNT}
	done
    done
done

csv_end perf
