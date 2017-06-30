#!/usr/bin/sh

cat $0
echo "-------------------------------"
date
uname -a
git describe
echo "-------------------------------"

export LD_PRELOAD=`pwd`/../../preload/pip_preload.so

NTASKS=50

doeval() {
    for ITER in 1 2 3 4 5 6 7 8 9 10 11 12
    do
	echo -n "["$ITER"]" $1:$2 " "
#	numactl -C 3 ./$1 $4 $3
	numactl -C 3 ./$1 $3
    done
    echo
}

#for TOUCH in 0 1 2 4 8 16 32 64 128 256 512 1024 2048 4096
for TOUCH in 1 2 4 8 16 32
do
#    for PROG in ctxsw-pip ctxsw-thread ctxsw-forkonly
    for PROG in futex-pip futex-thread futex-forkonly
    do
#	if [ $PROG == ctxsw-pip ]
	if [ $PROG == futex-pip ]
	then
	    for PIPMODE in process:preload process:pipclone thread
	    do
		doeval $PROG $PIPMODE $TOUCH $NTASKS
	    done
	else
	    doeval $PROG "" $TOUCH $NTASKS;
	fi
    done
    echo
done
