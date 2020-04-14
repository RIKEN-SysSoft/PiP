#!/bin/sh

tstp=$0
dir=`dirname $0`
. $dir/../exit_code.sh.inc

./pipcc --piproot root.c -o root
if [ $? -ne 0 ]; then
    echo './pipcc --piproot root.c -o root -- FAIL'
    exit $EXIT_FAIL;
else
    echo './pipcc --piproot root.c -o root -- SUCCESS'
fi
./pip-check -r root
if [ $? -ne 0 ]; then
    echo './pip-check -r root -- FAIL'
    exit $EXIT_FAIL;
else
    echo './pip-check -r root -- SUCCESS'
fi
./pip-check -t root
if [ $? -eq 0 ]; then
    echo './pip-check -t root -- FAIL'
    exit $EXIT_FAIL;
else
    echo './pip-check -t root -- SUCCESS'
fi
./pipcc --piptask task.c -o task
if [ $? -ne 0 ]; then
    echo './pipcc --piptask task.c -o task -- FAIL'
    exit $EXIT_FAIL;
else
    echo './pipcc --piptask task.c -o task -- SUCCESS'
fi
./pip-check -t task
if [ $? -ne 0 ]; then
    echo './pip-check -t task -- FAIL'
    exit $EXIT_FAIL;
else
    echo './pip-check -t task -- SUCCESS'
fi
##ldso=`./pip-check --ldso`
##if [ "$ldso" != "" ]; then
##    ./pip-check -r task
##    if [ $? -eq 0 ]; then
##	echo './pip-check -r task -- FAIL'
##	exit $EXIT_FAIL;
##    else
##	echo './pip-check -r task -- SUCCESS'
##    fi
##fi

../util/timer 1 ./root
if [ $? -ne 0 ]; then
    echo '../util/timer 1 ./root -- FAIL'
    exit $EXIT_FAIL;
else
    echo '../util/timer 1 ./root -- SUCCESS'
fi

echo $tstp ": PASS"; echo;
exit $EXIT_PASS;
