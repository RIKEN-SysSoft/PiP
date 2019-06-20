#!/bin/sh
dir=`dirname $0`

export TASKMAX=`$dir/util/dlmopen_count -p`
export NCORES=`$MCEXEC $dir/util/ompnumthread`
export LD_PRELOAD=$dir/../preload/pip_preload.so
export LD_LIBRARY_PATH=$dir/../lib

time=2;  	# 2 secs
niters=1; 	# one iteration
quiet=0; 	# output messages
continue=0; 	# exit on error
keep=0; 	# delete log file
out=0;		# output program messages
#
# exit_code
#
EXIT_PASS=0
EXIT_FAIL=1
EXIT_XPASS=2		# passed, but it's unexpected. fixed recently?
EXIT_XFAIL=3		# failed, but it's expected. i.e. known bug
EXIT_UNRESOLVED=4	# cannot determine whether (X)?PASS or (X)?FAIL
EXIT_UNTESTED=5		# not tested, this test hasn't been written yet
EXIT_UNSUPPORTED=6	# not tested, this test cannot run in this environment
EXIT_KILLED=7		# killed by Control-C or something

function print_usage() {
    echo >&2 "Usage: `basename $cmd` [-ACLT] [-o|-O|-m|-M] [-t <SEC>] [-n <N>] <test-prog>";
    exit 2;
}

function mesg() {
    if [ $quiet -eq 0 ]; then
	echo $@;
    fi
}

function mesg_n() {
    if [ $quiet -eq 0 ]; then
	echo -n $@ '';
    fi
}

function prt_ext() {
    exit=$1;
    tty > /dev/null 2>&1;
    if [ $? ]; then
	case $exit in
	    0) mesg_n "EXIT_PASS";;
	    1) mesg_n "EXIT_FAIL";;
	    2) mesg_n "EXIT_XPASS";;
	    3) mesg_n "EXIT_XFAIL";;
	    4) mesg_n "EXIT_UNRESOLVED";;
	    5) mesg_n "EXIT_UNTESTED";;
	    6) mesg_n "EXIT_UNSUPPORTED";;
	    7) mesg_n "EXIT_KILLED";;
	    *) mesg_n "(unknown:" $1 ")";;
	esac
    fi
    if [ $continue -eq 0 ]; then
	if [ $keep -ne 0 ]; then
	    mesg_n '' "($savefile)"
	fi
    fi
    echo;
    if [ $continue -eq 0 ] ; then
	exit $exit;
    fi
}

function control_c() {
    echo '^C';
    exit $EXIT_KILLED;
}

# parse command line option
cmd=$0;
case $# in
0)	print_usage;;
*)	test_mode_L=''
	test_mode_C=''
	test_mode_T=''
	test_task_a=''
	test_task_A=''
	test_task_m=''
	test_task_M=''
	while	case $1 in
		-*) true;;
		*) false;;
		esac
	do
		case $1 in -C*)	test_mode_C=C;; esac
		case $1 in -L*)	test_mode_L=L;; esac
		case $1 in -T*)	test_mode_T=T;; esac
		case $1 in -a) test_task_a=a;; esac
		case $1 in -A) test_task_A=A;; esac
		case $1 in -m) test_task_m=m;; esac
		case $1 in -M) test_task_M=M;; esac
		case $1 in -t) shift; time=$1;; esac
		case $1 in -n) shift; niters=$1;; esac
		case $1 in -q) quiet=1;; esac
		case $1 in -c) continue=1;; esac
		case $1 in -C) continue=1; keep=1;; esac
		case $1 in -x) out=1;; esac
		shift;
	done
	pip_mode_list="$test_mode_L $test_mode_C $test_mode_T";
	pip_task_list="$test_task_a $test_task_A $test_task_m $test_task_M"
	;;
esac
case $# in
0)	print_usage;;
*)	TESTPROG=$1;;
esac

if [ ! -x $TESTPROG ]; then
    echo $TESTPROG is NOT executable;
    exit 2;
fi

pip_modes=`echo $pip_mode_list | sed 's/ //'`;
if [[ x$pip_modes == x ]]; then
    pip_mode_list="C L T";
fi

pip_tasks=`echo $pip_task_list | sed 's/ //'`;
if [[ x$pip_tasks == x ]]; then
    pip_task_list="a A m M";
fi

PROGNAM=`basename $TESTPROG`;
logfile=.log-$PROGNAM

trap "echo; echo EXHIT_KILLED \(killed by user\); exit $EXIT_KILLED" 2;

extval=0
for ((i=0; i<$niters; i++));
do
    for task in $pip_task_list
    do
	case $task in
	    a) nacts=$NCORES; npass=0;;
	    A) nacts=$TASKMAX; npass=0;;
	    m) nacts=$NCORES; npass=$(($TASKMAX - $NCORES));;
	    M) nacts=$(($TASKMAX / 2)); npass=$(($TASKMAX - $nacts));;
	    *) nacts=$NCORES; npass=0;;
	esac
	for mode in $pip_mode_list
	do
	    case $mode in
		L) export PIP_MODE=process:preload;;
		C) export PIP_MODE=process:pipclone;;
		T) export PIP_MODE=thread;;
		*) export PIP_MODE=process;;
	    esac
	    mesg_n "[$i/$niters-$task:$mode] $TESTPROG $time $nacts $npass ... ";
	    if [ $out -ne 0 ]; then
		$MCEXEC $TESTPROG $time $nacts $npass;
	    else
		$MCEXEC $TESTPROG $time $nacts $npass > $logfile 2>&1;
	    fi
	    extval=$?;
	    if [ $extval -ne 0 ]; then
		savefile=test-$PROGNAM-$task:$mode:$i.log
		mv $logfile $savefile;
		prt_ext $extval;
		if [ ! $keep ]; then
		    rm $savefile;
		fi
	    else
		mesg done;
	    fi
	    rm -f $logfile;
	done
    done
done
prt_ext $extval;
