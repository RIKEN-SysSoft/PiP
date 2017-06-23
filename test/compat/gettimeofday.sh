#!/bin/sh

. ../test.sh.inc

trap 'rm -f $TEST_TMP; exit $EXIT_KILLED' $TEST_TRAP_SIGS

expected0=$(date +'%s')
expected1=$(expr $expected0 + 1)
expected2=$(expr $expected0 + 2)
expected3=$(expr $expected0 + 3)
expected4=$(expr $expected0 + 4)
expected5=$(expr $expected0 + 5)

./gettimeofday >$TEST_TMP 2>&1
if [ $(egrep "Hello, I am fine \(($expected0|$expected1|$expected2|$expected3|$expected4|$expected5):" \
	<$TEST_TMP | wc -l) -eq $TEST_PIP_TASKS ]
then
	test_exit_status=$EXIT_PASS
fi
rm -f $TEST_TMP
exit $test_exit_status
