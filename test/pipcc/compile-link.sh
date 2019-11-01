#!/bin/sh

dir=`dirname $0`
. $dir/../exit_code.sh.inc

testdir=$dir/../..

./pipcc --TEST $testdir -g --piproot root.c -o root;
if [ $? -ne 0 ]; then
    exit $EXIT_FAIL;
fi
./pipcheck -r root
if [ $? -ne 0 ]; then
    exit $EXIT_FAIL;
fi
./pipcheck -t root
if [ $? -eq 0 ]; then
    exit $EXIT_FAIL;
fi
./pipcc --TEST $testdir -g --piptask task.c -o task;
if [ $? -ne 0 ]; then
    exit $EXIT_FAIL;
fi
./pipcheck -t task
if [ $? -ne 0 ]; then
    exit $EXIT_FAIL;
fi
ldso=`./pipcheck --ldso`;
if [ "$ldso" != "" ]; then
    ./pipcheck -r task
    if [ $? -eq 0 ]; then
	exit $EXIT_FAIL;
    fi
fi

./root;

if [ $? -ne 0 ]; then
    exit $EXIT_FAIL;
fi
exit $EXIT_PASS;
