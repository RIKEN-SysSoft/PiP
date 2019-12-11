#!/bin/sh

tstp=$0
dir=`dirname $0`
. $dir/../exit_code.sh.inc

testdir=$dir/../..

./pipcc --pipdir $testdir -g -c ../../sample/hello/hello.c -o hello.o;
./pipcc --pipdir $testdir -g hello.o -o hello;
./pip-check -b hello

./hello;

rv=$?;
if [ $rv -ne 0 ]; then
    exit $EXIT_FAIL;
fi

echo $tstp ": PASS"; echo;
exit $EXIT_PASS;
