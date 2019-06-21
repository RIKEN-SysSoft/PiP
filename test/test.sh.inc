
# XXX TO-DO: LC_ALL=en_US.UTF-8 doesn't work if custom-built libc is used
unset LANG LC_ALL

export PATH=.:./util:$PATH

TEST_TMP=/tmp/piptest$$
TEST_TRAP_SIGS='1 2 15'

: ${TEST_PIP_TASKS:=$(dlmopen_count -p)}

export NTASKS=${TEST_PIP_TASKS}
export OMP_NUM_THREADS=`$MCEXEC ompnumthread`
export LD_PRELOAD=$dir/../preload/pip_preload.so

test_exit_status=$EXIT_FAIL	# defaut exit status

test_msg_count()
{
	local msg
	local expected_count

	case $# in
	0)	echo >&2 "$0: Usage: test_msg_count <msg> [<expected_count>]"
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
