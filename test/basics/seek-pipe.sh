#!/bin/sh
dir=`dirname $0`
. $dir/../exit_code.sh.inc
$dir/../util/pip_is_debug_build;
#if [ $? -ne 0 ] ; then
#    exit $EXIT_UNTESTED;
#fi
ntasks=$1;
niters=100
$dir/seek stdout $1 100 | $dir/seek stdin $ntasks $niters
exit $?
