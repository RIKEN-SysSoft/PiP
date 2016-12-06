#!/bin/sh

. ../test.sh.inc

trap 'rm -f $TEST_TMP; exit $EXIT_KILLED' $TEST_TRAP_SIGS

test_exit_status=$EXIT_FAIL

./main >$TEST_TMP 2>&1
if [ $(grep '^MAIN$'   <$TEST_TMP | wc -l) -eq 1 ] &&
   [ $(grep '^CHILD$' <$TEST_TMP | wc -l) -eq 1 ]
then
	test_exit_status=$EXIT_PASS
fi
rm -f $TEST_TMP
exit $test_exit_status
