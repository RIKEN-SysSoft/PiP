#!/bin/sh

. ../test.sh.inc

trap 'rm -f $TEST_TMP; exit $EXIT_KILLED' $TEST_TRAP_SIGS

./gethostbyname >$TEST_TMP 2>&1
if [ $(egrep '^hostname: localhost(\.localdomain)?$' <$TEST_TMP | wc -l) -eq 2 ] &&
   [ $(egrep '^hostname: 127\.0\.0\.1$' <$TEST_TMP | wc -l) -eq 2 ]
then
	test_exit_status=$EXIT_PASS
fi
rm -f $TEST_TMP
exit $test_exit_status
