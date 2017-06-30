#!/usr/bin/sh

#echo 10 > /proc/sys/vm/nr_hugepages

cat $0
echo "-------------------------------"
date
uname -a
git describe
cat /proc/meminfo | grep Huge
echo "-------------------------------"
echo

export LD_PRELOAD=`pwd`/../../preload/pip_preload.so

for PIPMODE in process:preload process:pipclone thread
do
    export PIP_MODE=$PIPMODE
    echo "#### PIP ($PIP_MODE) small-TLB"
    numactl -C 3 ./pip
    echo "-------------------------------"
    echo
done

echo "#### XPMEM small-TLB"
numactl -C 3 ./xpmem
echo "-------------------------------"
echo

echo "#### SHMEM small-TLB"
numactl -C 3 ./shmem
echo "-------------------------------"
echo

for PIPMODE in process:preload process:pipclone thread
do
    export PIP_MODE=$PIPMODE
    echo "#### PIP ($PIP_MODE) LARGE-TLB"
    numactl -C 3 ./pip-ht
    echo "-------------------------------"
    echo
done

echo "#### XPMEM LARGE-TLB"
numactl -C 3 ./xpmem-ht
echo "-------------------------------"
echo
