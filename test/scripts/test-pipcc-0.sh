#!/bin/sh

tstp=$0;
dir=`dirname $0`;
. $dir/../exit_code.sh.inc;

testdir=$dir/../..;
hello="hello";

check_rv () {
    if [ $1 -ne 0 ]; then
	if -f ${hello}; then
	    rm ${hello} > /dev/null 2>&1;
	fi
	if -f ${hello}.o; then
	    rm ${hello}.o > /dev/null 2>&1;
	fi
	exit $EXIT_FAIL;
    fi
}

./pipcc -g -c ${hello}.c -o ${hello}.o;
check_rv $?;
./pipcc -g ${hello}.o -o ${hello};
check_rv $?;
./pip-check -b ${hello}
check_rv $?;

../util/timer 1 ./${hello};
check_rv $?;

echo $tstp ": PASS"; echo;
exit $EXIT_PASS;
