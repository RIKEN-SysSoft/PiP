#!/bin/sh

dir=`dirname $0`
. $dir/exit_code.sh.inc

mode_env=$PIP_MODE;
mode_arg=$1;
shift;

if [[ x$PIP_MODE != x ]]; then
    case $mode_arg in
	*C*)
	    case $mode_env in
		process:pipclone) 	skip_flag=1;;
	    esac;;
	*L*)
	    case $mode_env in
		process:preload) 	skip_flag=1;;
	    esac;;
	*T*)
	    case $mode_env in
		pthread)	 	skip_flag=1;;
	    esac;;
	*P*)
	    case $mode_env in
		process:pipclone) 	skip_flag=1;;
		process:preload)	skip_flag=1;;
	    esac;;
	*process:pipclone*)
	    case $mode_env in
		process:pipclone) 	skip_flag=1;;
	    esac;;
	*process:preload*)
	    case $mode_env in
		process:preload) 	skip_flag=1;;
	    esac;;
	*process*)
	    case $mode_env in
		process:pipclone) 	skip_flag=1;;
		process:preload)	skip_flag=1;;
	    esac;;
	*thread*)
	    case $mode_env in
		pthread)	 	skip_flag=1;;
	    esac;;
    esac
fi

if [[ x$skip_flag != x ]]; then
    exit $EXIT_UNSUPPORTED;
fi

exec $@;
