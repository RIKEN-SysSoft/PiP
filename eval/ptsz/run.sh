#!/usr/bin/sh

cat $0
echo "-------------------------------"
date
uname -a
git describe
echo "-------------------------------"

export LD_PRELOAD=`pwd`/../../preload/pip_preload.so

PROGS="mmap-pip mmap-thread mmap-forkonly"
NTASKS=50

doeval() {
    if [ $1 == mmap-pip ]
    then
	for PIPMODE in process:preload process:pipclone thread
	do
	    export PIP_MODE=$PIPMODE;
	    #for ITER in 1 2 3 4 5 6 7 8 9 10 11 12
	    for ITER in 1 2 3 4
	    do
		echo "["$ITER"]" $1:$PIPMODE $2 [Tasks] $3 "[MB]"
		./$1 $2 $3
	    done
	done
    else
	#for ITER in 1 2 3 4 5 6 7 8 9 10 11 12
	for ITER in 1 2 3 4 5 6
	do
	    echo "["$ITER"]" $1 $2 [Tasks] $3 "[MB]"
	    ./$1 $2 $3
	done
    fi
}

for NTASKS in 10 20 30 40 50 60 70 80 90 100
do
    for PROG in ${PROGS}
    do
	doeval ${PROG} ${NTASKS} 8 $ITER
    done
done
