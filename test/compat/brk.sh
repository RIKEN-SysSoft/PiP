#!/bin/sh

. ../test.sh.inc

trap 'rm -f $TEST_TMP; exit $EXIT_KILLED' $TEST_TRAP_SIGS

./brk >$TEST_TMP 2>&1
if [ $(grep '^<.* brk:' <$TEST_TMP | wc -l) \
	-eq $(expr $TEST_PIP_TASKS + 1) ] &&
   [ $(awk '$0 ~ /^<.* brk:/ { print $2, $3}' $TEST_TMP | sort -u | wc -l) \
	-eq 1 ]
then
	test_exit_status=$EXIT_PASS
fi
rm -f $TEST_TMP
exit $test_exit_status
