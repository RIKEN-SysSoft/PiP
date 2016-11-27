TEST_TMP=/tmp/piptest$$
TEST_TRAP_SIGS='1 2 15'

: ${TEST_PIP_TASKS:=$(../util/dlmopen_count)}

#
# exit_code
#
EXIT_PASS=0
EXIT_FAIL=1
EXIT_XPASS=2		# passed, but it's unexpected. fixed recently?
EXIT_XFAIL=3		# failed, but it's expected. i.e. known bug
EXIT_UNRESOLVED=4	# cannot determine whether (X)?PASS or (X)?FAIL
EXIT_UNTESTED=5		# not tested, this test haven't written yet
EXIT_UNSUPPORTED=6	# not tested, this environment can't test this
EXIT_KILLED=7		# killed by Control-C or something

test_exit_status=$EXIT_FAIL	# defaut exit status

test_msg_count()
{
	local msg
	local expected_count

	case $# in
	0)	echo >&2 "$0: Usage: test.msg_count <msg> [<expected_count>]"
		return $EXIT_UNRESOLVED;;
	1)	msg=$1; expected_count=$TEST_PIP_TASKS;;
	2)	msg=$1; expected_count=$2;;
	esac

	if [ $(cat | fgrep "$msg" | wc -l) -eq $expected_count ]
	then
		test_exit_status=$EXIT_PASS
	fi
	return $test_exit_status
}
