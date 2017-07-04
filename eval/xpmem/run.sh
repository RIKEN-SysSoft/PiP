#!/bin/sh

. ../eval.sh.inc

#echo 10 > /proc/sys/vm/nr_hugepages

for PIPMODE in $MODE_LIST
do
    export PIP_MODE=$PIPMODE
    csv_begin PIP-small-$PIPMODE
    echo "#### PIP ($PIP_MODE) small-TLB"
    NUMACTL 3 ./pip
    csv_end PIP-small-$PIPMODE
    echo
done

if [ -x ./xpmem ]
then
    csv_begin XPMEM-small
    echo "#### XPMEM small-TLB"
    NUMACTL 3 ./xpmem
    csv_end XPMEM-small
    echo
fi

csv_begin SHMEM-small
echo "#### SHMEM small-TLB"
NUMACTL 3 ./shmem
csv_end SHMEM-small
echo

if [ `cat /proc/sys/vm/nr_hugepages` -gt 0 ]
then
    for PIPMODE in $MODE_LIST
    do
	export PIP_MODE=$PIPMODE
	csv_begin PIP-huge-$PIPMODE
	echo "#### PIP ($PIP_MODE) LARGE-TLB"
	NUMACTL 3 ./pip-ht
	csv_end PIP-huge-$PIPMODE
	echo
    done
    if [ -x ./xpmem-ht ]
    then
	csv_begin XPMEM-huge
	echo "#### XPMEM LARGE-TLB"
	NUMACTL 3 ./xpmem-ht
	csv_end XPMEM-huge
	echo
    fi
else
    csv_begin Error-Message
    echo "NO HugTLB pages allocated. To allocate HugeTLB pages:"
    echo "   sudo echo 10 > /proc/sys/vm/nr_hugepages"
    csv_end Error-Message
fi
