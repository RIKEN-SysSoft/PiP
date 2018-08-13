#!/bin/sh

. ../test.sh.inc

trap 'rm -f $TEST_TMP; exit $EXIT_KILLED' $TEST_TRAP_SIGS

# redmine #508
#$MCEXEC ./stack >$TEST_TMP 2>&1
$MCEXEC ./stack 2>&1 | tee $TEST_TMP >/dev/null
if [ $(grep ' stack=' <$TEST_TMP | wc -l) -eq $TEST_PIP_TASKS ]
then
	test_exit_status=$EXIT_PASS
fi

case `../util/pip_mode` in
process:pipclone|pthread)
	case $test_exit_status in
	$EXIT_PASS)	test_exit_status=$EXIT_PASS;;
	$EXIT_FAIL)	test_exit_status=$EXIT_XFAIL;; # redmine #508
	esac
esac

rm -f $TEST_TMP
exit $test_exit_status
