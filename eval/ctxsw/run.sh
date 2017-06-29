#!/usr/bin/sh

cat $0
echo "-------------------------------"
date
uname -a
git describe
echo "-------------------------------"

export LD_PRELOAD=/home/ahori/work/PIP/PIP/preload/pip_preload.so

NTASKS=50

for TOUCH in 0 1 2 4 8 16 32 64 128 256 512 1024 2048 4096
do
    for PROG in ctxsw-pip ctxsw-thread ctxsw-forkonly
    do
	if [ $PROG == ctxsw-pip ]
	then export LD_PRELOAD=/home/ahori/work/PIP/PIP/preload/pip_preload.so
	else unset LD_PRELOAD
	fi
	for ITER in 1 2 3 4 5 6 7 8 9 10 11 12
	do
	    echo -n "["$ITER"]" $PROG "("$TOUCH")"": "
	    numactl -C 0 ./$PROG $NTASKS $TOUCH
	done
	echo
    done
    echo
done
