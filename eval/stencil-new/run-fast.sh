#!/bin/sh
#PJM -L node=1
#PJM -L elapse=1:00:00
#PJM -L rscgrp=regular-cache
#PJM -L node=1
#PJM -g gg10
#PJM -j

## LD_LIBRARY_PATH=/work/0/gg10/g10013/work/PIP/import/mpich-install/lib/:/work/0/gg10/g10013/work/PIP/import/install/lib:$LD_LIBRARY_PATH /work/0/gg10/g10013/work/PIP/import/mpich-install/bin/mpiexec -n 16 ./stencil_pip 4096 1 1000

. ./eval.sh.inc

PROGS="stencil_pip_fast stencil_mpi_fast"
#SIZE=1024
#SIZE=4096
SIZE=8192
ENERGY=1
NTASKS=64

NCPUS=`grep CPU /proc/cpuinfo | wc -l`
MODES="process"

MPIRUN_PIP=/work/gg10/share/ahori/PIP/install-pip/bin/mpiexec
MPIRUN_MPI=/work/gg10/share/ahori/PIP/install-org/bin/mpiexec

LIBS=$LD_LIBRARY_PATH
PIPLIB=/work/gg10/share/ahori/PIP/install-pip/lib
PIPMPILIB=/work/gg10/share/ahori/PIP/install-pip/lib
MPIMPILIB=/work/gg10/share/ahori/PIP/install-org/lib

MPIOPTS="-bind-to core:1"

##ITER_NUM="1 2"

doeval() {
    if [ $1 == stencil_pip_fast ]
    then
	export PIPMODE=process;
	export LD_LIBRARY_PATH=$PIPMPILIB:$PIPLIB:$LIBS;
#	echo $MPIRUN_PIP;
#	echo $LD_LIBRARY_PATH;
	export LD_PRELOAD=`pwd`/../../preload/pip_preload.so
	for ITER in $ITER_NUM
	do
	    echo -n "["$ITER"]" $1:$PIPMODE $2 " ";
	    $MPIRUN_PIP -np $NTASKS $MPIOPTS ./$1 $SIZE $ENERGY $2;
	done
    else
	unset PIP_MODE;
	unset LD_PRELOAD;
	export LD_LIBRARY_PATH=$MPIMPILIB:$LIBS
#	echo $MPIRUN_MPI;
#	echo $LD_LIBRARY_PATH;
	for ITER in $ITER_NUM
	do
	    echo -n "["$ITER"]" $1 $2 " "
	    $MPIRUN_MPI -np $NTASKS $MPIOPTS ./$1 $SIZE $ENERGY $2;
	done
    fi
    echo
}

csv_begin

for NITERS in 10 20 40 80 160 320 640 1280
do
    if [ $NTASKS -gt $NCPUS ]
    then
	 break
    fi
    if [ $NTASKS -gt $CORENUM ]
    then
	break
    fi
    for PROG in ${PROGS}
    do
	doeval ${PROG} ${NITERS}
    done
done

csv_end
