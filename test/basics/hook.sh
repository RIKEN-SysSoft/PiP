#!/bin/sh

. ../test.sh.inc

trap 'rm -f $TEST_TMP; exit $EXIT_KILLED' $TEST_TRAP_SIGS

./hook >$TEST_TMP 2>&1
if fgrep 'before hook is called' $TEST_TMP >/dev/null &&
   fgrep 'Hello, I am fine !!' $TEST_TMP >/dev/null &&
   fgrep 'after hook is called' $TEST_TMP >/dev/null
then
	test_exit_status=$EXIT_PASS
fi
rm -f $TEST_TMP
exit $test_exit_status
