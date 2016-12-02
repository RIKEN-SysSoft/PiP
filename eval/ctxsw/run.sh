#!/usr/bin/sh

export LD_PRELOAD=/home/ahori/work/PIP/PIP/preload/pip_preload.so

NTASKS=50

for TOUCH in 0 1 2 4 8 16 32 64 128 256
do
for PROG in ctxsw-pip ctxsw-thread ctxsw-forkonly
do
    ./$PROG $NTASKS $TOUCH
done
done
