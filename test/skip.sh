#!/bin/sh

dir=`dirname $0`
. $dir/exit_code.sh.inc

prog=`basename $0`;

mode_env=$PIP_MODE;
extval=$EXIT_UNTESTED;

usage()
{
    echo $prog "<MODE> <EXIT_CODE> a.out ...";
    exit $extval;
}

mode_arg=$1;
shift;
exit_code=$1;
shift;

if [ $# -eq 0 ]; then
    usage;
fi
if [ x"$PIP_MODE" = x ]; then
    usage;
fi

case $mode_arg in
    *A*) 			skip_flag=skip;;
    *C*)
	case $mode_env in
	    process:pipclone) 	skip_flag=skip;;
	esac;;
    *L*)
	case $mode_env in
	    process:preload) 	skip_flag=skip;;
	esac;;
    *T*)
	case $mode_env in
	    pthread)	 	skip_flag=skip;;
	esac;;
    *P*)
	case $mode_env in
	    process:pipclone) 	skip_flag=skip;;
	    process:preload)	skip_flag=skip;;
	esac;;
    *process:pipclone*)
	case $mode_env in
	    process:pipclone) 	skip_flag=skip;;
	esac;;
    *process:preload*)
	case $mode_env in
	    process:preload) 	skip_flag=skip;;
	esac;;
    *process*)
	case $mode_env in
	    process:pipclone) 	skip_flag=skip;;
	    process:preload)	skip_flag=skip;;
	esac;;
    *thread*)
	case $mode_env in
	    pthread)	 	skip_flag=skip;;
	esac;;
    *D*)
	$dir/util/pip_is_debug_build;
	if [ $? != 0 ]; then
	    skip_debug=skip;
	fi;;
    *) usage;
esac

case $exit_code in
    EXIT_PASS		| PASS) 	extval=$EXIT_PASS;;
    EXIT_FAIL 		| FAIL)		extval=$EXIT_FAIL;;
    EXIT_XPASS		| XPASS)	extval=$EXIT_XPASS;;
    EXIT_XFAIL 		| XFAIL) 	extval=$EXIT_XFAIL;;
    EXIT_UNRESOLVED 	| UNRESOLVED)	extval=$EXIT_UNRESOLVED;;
    EXIT_UNTESTED 	| UNTESTED)	extval=$EXIT_UNTESTED;;
    EXIT_UNSUPPORTED 	| UNSUPPORTED)	extval=$EXIT_UNSUPPORTED;;
    EXIT_KILLED 	| KILLED)	extval=$EXIT_KILLED;;
    *) 			echo $exit_code "unknown"; exit $extval;;
esac

if [ x"$skip_debug" = x"skip" ]; then
    exit $extval;
fi
if [ x"$skip_flag" = x"skip" ]; then
    exit $extval;
fi

exec "$@";
