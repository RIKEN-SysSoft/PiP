#!/bin/sh

export LD_PRELOAD=`pwd`/../preload/pip_preload.so

# XXX TO-DO: LC_ALL=en_US.UTF-8 doesn't work if custom-built libc is used
unset LANG LC_ALL

: ${TEST_PIP_TASKS:=$(./util/dlmopen_count -p)}

print_summary()
{
	echo ""
	echo     "Total test: $(expr $n_PASS + $n_FAIL + $n_XPASS + $n_XFAIL \
		+ $n_UNRESOLVED + $n_UNTESTED + $n_UNSUPPORTED + $n_KILLED)"
	echo     "  success            : $n_PASS"
	echo     "  failure            : $n_FAIL"

	if [ $n_XPASS -gt 0 ]; then
	    echo "  unexpected success : $n_XPASS"
	fi
	if [ $n_XFAIL -gt 0 ]; then
	    echo "  expected failure   : $n_XFAIL"
	fi
	if [ $n_UNRESOLVED -gt 0 ]; then
	    echo "  unresolved         : $n_UNRESOLVED"
	fi
	if [ $n_UNTESTED -gt 0 ]; then
	    echo "  untested           : $n_UNTESTED"
	fi
	if [ $n_UNSUPPORTED -gt 0 ]; then
	    echo "  unsupported        : $n_UNSUPPORTED"
	fi
	if [ $n_KILLED -gt 0 ]; then
	    echo "  killed             : $n_KILLED"
	fi
}

. ./test.sh.inc

TEST_LIST=test.list
TEST_LOG_FILE=test.log

mv -f ${TEST_LOG_FILE} ${TEST_LOG_FILE}.bak 2>/dev/null

pip_mode_list_all='L C T'
pip_mode_name_P=process
pip_mode_name_L=$pip_mode_name_P:preload
pip_mode_name_C=$pip_mode_name_P:pipclone
pip_mode_name_T=pthread

function print_mode_list() {
    echo "List of PiP execution modes:"
    echo "  " $pip_mode_name_T "(T)";
    echo "  " $pip_mode_name_P "(P)";
    echo "  " $pip_mode_name_L "(L)";
    echo "  " $pip_mode_name_C "(C)";
    exit 1;
}

function print_usage() {
    echo >&2 "Usage: `basename $cmd` [-APCLT] [-thread] [-process[:preload|:pipclone]] [<test_list_file>]";
    exit 2;
}

# parse command line option
cmd=$0
case $# in
0)	print_usage;;
*)	run_test_L=''
	run_test_C=''
	run_test_T=''
	while	case $1 in
		-*) true;;
		*) false;;
		esac
	do
		case $1 in *A*)	run_test_L=L; run_test_C=C; run_test_T=T;; esac
		case $1 in *P*)	run_test_L=L; run_test_C=C;; esac
		case $1 in *C*)	run_test_C=C;; esac
		case $1 in *L*)	run_test_L=L;; esac
		case $1 in *T*)	run_test_T=T;; esac
		case $1 in *list*) print_mode_list;; esac
		case $1 in *thread)    run_test_T=T;; esac
		case $1 in *process)   run_test_L=L; run_test_C=C;; esac
		case $1 in *$pip_mode_name_L) run_test_L=L;; esac
		case $1 in *$pip_mode_name_C) run_test_C=C;; esac
		shift
	done
	pip_mode_list="$run_test_L $run_test_C $run_test_T"
	;;
esac
case $# in
0)	;;
1)	TEST_LIST=$1;;
*)	echo >&2 "`basename $cmd`: unknown argument '$*'"
	print_usage
	exit 2;;
esac

if [ -z "$pip_mode_list" ]; then
    print_usage;
fi


echo LD_LIBRARY_PATH=$LD_LIBRARY_PATH
echo LD_PRELOAD=$LD_PRELOAD
echo NTASKS: ${TEST_PIP_TASKS}

# check whether each $PIP_MODE is testable or not
run_test_L=''
run_test_C=''
run_test_T=''
for pip_mode in $pip_mode_list
do
	eval 'pip_mode_name=$pip_mode_name_'${pip_mode}
	case `PIP_MODE=$pip_mode_name ./util/pip_mode 2>/dev/null` in
	$pip_mode_name)
		eval "run_test_${pip_mode}=${pip_mode}"
		echo "testing ${pip_mode} - ${pip_mode_name}"
		;;
	*)	echo >&2 "WARNING: $pip_mode_name is not testable";;
	esac
done
pip_mode_list="$run_test_L $run_test_C $run_test_T"

echo

n_PASS=0
n_FAIL=0
n_XPASS=0
n_XFAIL=0
n_UNRESOLVED=0
n_UNTESTED=0
n_UNSUPPORTED=0
n_KILLED=0

LOG_BEG="=== ============================================================ ===="
LOG_SEP="--- ------------------------------------------------------------ ----"

while read line; do
	set x $line
	shift
	case $# in 0) continue;; esac
	case $1 in '#'*) continue;; esac

	test=$1

	for pip_mode in $pip_mode_list
	do
		eval 'pip_mode_name=$pip_mode_name_'${pip_mode}

		printf "%-60.60s ${pip_mode} ..." $test
		(
		  echo "$LOG_BEG"
		  echo "--- $test PIP_MODE=${pip_mode_name}"
		  echo "$LOG_SEP"
		  date +'@@_ start at %s - %Y-%m-%d %H:%M:%S'
		) >>$TEST_LOG_FILE

		if [ ! -x $test ]; then
			test_exit_status=$EXIT_UNTESTED
		else
			(
				if cd $(dirname $test)
				then
					PIP_MODE=$pip_mode_name \
						./$(basename $test) \
						</dev/null >>$TEST_LOG_FILE 2>&1
				else
					exit $EXIT_UNTESTED
				fi
			)
			test_exit_status=$?
		fi

		msg=
		case $test_exit_status in
		$EXIT_PASS)		status=PASS;;
		$EXIT_FAIL)		status=FAIL;;
		$EXIT_XPASS)		status=XPASS;;
		$EXIT_XFAIL)		status=XFAIL;;
		$EXIT_UNRESOLVED)	status=UNRESOLVED;;
		$EXIT_UNTESTED)		status=UNTESTED;;
		$EXIT_UNSUPPORTED)	status=UNSUPPORTED;;
		$EXIT_KILLED)		status=KILLED;;
		*)			status=UNRESOLVED
					msg="exit status $test_exit_status";;
		esac
		: ${msg:=$status}
		eval "((n_$status=n_$status + 1))"

		(
		  date +'@@~  end  at %s - %Y-%m-%d %H:%M:%S'
		  echo "$LOG_SEP"
		  printf "@:= %-60.60s %s\n" $test "$msg"
		) >>$TEST_LOG_FILE

		echo " $msg"

		# stopped by Control-C or something like that
		[ $test_exit_status -eq $EXIT_KILLED ] && break 2
	done

done < $TEST_LIST

(
	echo $LOG_BEG
	print_summary
) >>$TEST_LOG_FILE

print_summary

case $n_KILLED in 0) :;; *) exit $EXIT_KILLED;; esac
[ $n_FAIL -eq 0 -a $n_UNRESOLVED -eq 0 ]
