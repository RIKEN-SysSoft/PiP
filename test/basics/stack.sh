#!/bin/sh

. ../test.sh.inc

trap 'rm -f $TEST_TMP; exit $EXIT_KILLED' $TEST_TRAP_SIGS

./stack >$TEST_TMP 2>&1
if [ $(grep ' stack=' <$TEST_TMP | wc -l) -eq $TEST_PIP_TASKS ]
then
	test_exit_status=$EXIT_PASS
fi
rm -f $TEST_TMP
exit $test_exit_status
