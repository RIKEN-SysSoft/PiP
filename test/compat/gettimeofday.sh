#!/bin/sh

. ../test.sh.inc

trap 'rm -f $TEST_TMP; exit $EXIT_KILLED' $TEST_TRAP_SIGS

expected0=$(date +'%s')
expected1=$(expr $expected0 + 1)
expected2=$(expr $expected0 + 2)

./gettimeofday >$TEST_TMP 2>&1
if [ $(egrep "Hello, I am fine \(($expected0|$expected1|$expected2):" \
	<$TEST_TMP | wc -l) -eq $TEST_PIP_TASKS ]
then
	test_exit_status=$EXIT_PASS
fi
rm -f $TEST_TMP
exit $test_exit_status
