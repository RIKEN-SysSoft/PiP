#!/bin/sh

. ../eval.sh.inc

PROGS="stencil_pip stencil_mpi"
FPROGS="stencil_pip_fast stencil_mpi_fast"
SIZE=4096
ENERGY=1
NITERS=1000
PIPRUN=`pwd`/../../bin/piprun
<<<<<<< HEAD
NCPUS=`grep CPU /proc/cpuinfo | wc -l`
=======
MODES="process"
>>>>>>> d438bc49f4a60e049b0a1d4415b7a14524cbe35a

dofast() {
    if [ $1 == stencil_pip_fast ]
    then
	for PIPMODE in $MODES
	do
	    export PIP_MODE=$PIPMODE;
	    for ITER in $ITER_NUM
	    do
		echo -n "["$ITER"]" $1:$PIPMODE $2 "(" $3 ") "
		$PIPRUN -n $2 ./$1 $SIZE $ENERGY $3
	    done
	    echo
	done
    else
	for ITER in $ITER_NUM
	do
	    echo -n "["$ITER"]" $1 $2 "(" $3 ") "
	    mpirun -np $2 ./$1 $SIZE $ENERGY $3
	done
	echo
    fi
}

doeval() {
    if [ $1 == stencil_pip ]
    then
	for PIPMODE in $MODE_LIST
	do
	    export PIP_MODE=$PIPMODE;
	    for ITER in $ITER_NUM
	    do
		echo -n "["$ITER"]" $1:$PIPMODE $2 " "
		$PIPRUN -n $2 ./$1 $SIZE $ENERGY $NITERS
	    done
	    echo
	done
    else
	for ITER in $ITER_NUM
	do
	    echo -n "["$ITER"]" $1 $2 " "
	    mpirun -np $2 ./$1 $SIZE $ENERGY $NITERS
	done
	echo
    fi
}

<<<<<<< HEAD
csv_begin fast

for LCOUNT in 1 2 4 8 16 32 64 128
do
    for NTASKS in 1 2 4 8 16 32 64 128
    do
	if [ $NTASKS -gt $NCPUS ]
	then
	    break
	fi
	for PROG in ${FPROGS}
	do
	    dofast ${PROG} ${NTASKS} ${LCOUNT}
	done
    done
done

csv_end fast

csv_begin

for NTASKS in 1 2 4 8 16 32 64 128
=======
#csv_begin fast
#
#for LCOUNT in 1 2 4 8 16 32 64 128
#do
#    for NTASKS in 2 4 8 16 32 64 128 
#    do
#	if [ $NTASKS -gt $NTMAX ]
#	then
#	    break
#	fi
#	for PROG in ${FPROGS}
#	do
#	    dofast ${PROG} ${NTASKS} ${LCOUNT}
#	done
#    done
#done
#
#csv_end fast

csv_begin

for NTASKS in 2 4 8 16 32 64 128 
>>>>>>> d438bc49f4a60e049b0a1d4415b7a14524cbe35a
do
    if [ $NTASKS -gt $NCPUS ]
    then
	 break
    fi
    for PROG in ${PROGS}
    do
	doeval ${PROG} ${NTASKS}
    done
done

csv_end
