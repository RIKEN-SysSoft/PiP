#!/usr/bin/sh

cat $0
echo "-------------------------------"
date
uname -a
git describe
echo "-------------------------------"

export LD_PRELOAD=`pwd`/../../preload/pip_preload.so

NTASKS=10

doeval() {
    for ITER in 1 2 3 4 5 6 7 8 9 10 11 12
    do
	echo -n "["$ITER"]" $1:$2 " "
	./$1 $3 $4
    done
    echo
}

for SZ in 4 8 16 32 64 128 256 512 1024
do
    for PROG in mmap-pip mmap-thread mmap-forkonly
    do
	if [ $PROG == mmap-pip ]
	then
	    for PIPMODE in process:preload process:pipclone thread
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
