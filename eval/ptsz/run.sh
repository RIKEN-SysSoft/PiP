#!/usr/bin/sh

cat $0
echo "-------------------------------"
date
uname -a
git describe
echo "-------------------------------"

PROGS="mmap-pip mmap-thread mmap-forkonly"
NTASKS=50

doeval() {
    if [ $1 == mmap-pip ]
    then export LD_PRELOAD=/home/ahori/work/PIP/PIP/preload/pip_preload.so
    else unset LD_PRELOAD
    fi
    echo $1 $2 $3 LD_PRELOAD=${LD_PRELOAD}
    ./$1 $2 $3
}

for NTASKS in 10 20 30 40 50 60 70 80 90 100
do
for PROG in ${PROGS}
do
#for ITER in 1 2 3 4 5 6 7 8 9 10 11 12
for ITER in 1 2
do
doeval ${PROG} ${NTASKS} 8
done
done
done
