#!/bin/sh

dir=`dirname $0`
. $dir/../exit_code.sh.inc

testdir=$dir/../..

./pipcc --TEST $testdir -g -c ../../sample/hello/hello.c -o hello.o;
./pipcc --TEST $testdir -g hello.o -o hello;
./pipcheck -b hello

./hello;

rv=$?;
if [ $rv -ne 0 ]; then
    exit $EXIT_FAIL;
fi
exit $EXIT_PASS;
