#!/usr/bin/sh

PROGS="pt-pip pt-thread pt-forkonly"
NTASKS="20 25 30 35 40 45 50"

doeval() {
    if [ $1 == pt-pip ]
    then export LD_PRELOAD=/home/ahori/work/PIP/install/lib/pip_preload.so
    else unset LD_PRELOAD
    fi
    sleep 2
    echo $1 $2 $3 LD_PRELOAD=${LD_PRELOAD}
    ./$1 $2
}

for NT in ${NTASKS}
do
for PROG in ${PROGS}
do
for ITER in 1 2 3 4 5 6 7 8 9 10
do
doeval ${PROG} ${NT}
done
done
done

