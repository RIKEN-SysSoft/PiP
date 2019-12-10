#!/bin/sh

dir=`dirname $0`
. $dir/../exit_code.sh.inc

testdir=$dir/../..

./pipcc --TEST $testdir -g --piproot root.c -o root;
if [ $? -ne 0 ]; then
    exit $EXIT_FAIL;
fi
./pip-check -r root
if [ $? -ne 0 ]; then
    exit $EXIT_FAIL;
fi
./pip-check -t root
if [ $? -eq 0 ]; then
    exit $EXIT_FAIL;
fi
./pipcc --TEST $testdir -g --piptask task.c -o task;
if [ $? -ne 0 ]; then
    exit $EXIT_FAIL;
fi
./pip-check -t task
if [ $? -ne 0 ]; then
    exit $EXIT_FAIL;
fi
ldso=`./pip-check --ldso`;
if [ "$ldso" != "" ]; then
    ./pip-check -r task
    if [ $? -eq 0 ]; then
	exit $EXIT_FAIL;
    fi
fi

./root;

if [ $? -ne 0 ]; then
    exit $EXIT_FAIL;
fi
exit $EXIT_PASS;
