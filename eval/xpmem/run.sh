#!/bin/sh

. ../eval.sh.inc

#echo 10 > /proc/sys/vm/nr_hugepages

for PIPMODE in process:preload process:pipclone thread
do
    export PIP_MODE=$PIPMODE
    csv_begin PIP-small-$PIPMODE
    echo "#### PIP ($PIP_MODE) small-TLB"
    numactl -C 3 ./pip
    csv_end PIP-small-$PIPMODE
    echo
done


csv_begin XPMEM-small
echo "#### XPMEM small-TLB"
numactl -C 3 ./xpmem
csv_end XPMEM-small
echo

csv_begin SHMEM-small
echo "#### SHMEM small-TLB"
numactl -C 3 ./shmem
csv_end SHMEM-small
echo

if [ `cat /proc/sys/vm/nr_hugepages` -gt 0 ]
then
    for PIPMODE in process:preload process:pipclone thread
    do
	export PIP_MODE=$PIPMODE
	csv_begin PIP-huge-$PIPMODE
	echo "#### PIP ($PIP_MODE) LARGE-TLB"
	numactl -C 3 ./pip-ht
	csv_end PIP-huge-$PIPMODE
	echo
    done
    csv_begin XPMEM-huge
    echo "#### XPMEM LARGE-TLB"
    numactl -C 3 ./xpmem-ht
    csv_end XPMEM-huge
    echo
else
    csv_begin Error-Message
    echo "NO HugTLB pages allocated. To allocate HugeTLB pages:"
    echo "   sudo echo 10 > /proc/sys/vm/nr_hugepages"
    csv_end Error-Message
fi
