#!/usr/bin/sh

export LD_PRELOAD=/home/ahori/work/PIP/PIP/preload/pip_preload.so

NTASKS=50

echo PIP_MODE=`../../test/util/pip_mode`

for NTASKS in 1 2 4 8 16 32 64 128
do
    for PROG in spawn-pip spawn-forkexec spawn-vforkexec spawn-posix spawn-thread
    do
	if [ $PROG == spawn-pip ]
	fi
	for ITER in 1 2 3 4 5 6 7 8 9 10 11 12
	do
	    numactl -C 1 ./$PROG $NTASKS $NTASKS
	    #./$PROG $NTASKS $NTASKS
	done
	echo
    done
    echo
done

echo PIP_MODE=`../../test/util/pip_mode`
