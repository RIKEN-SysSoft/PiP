#!/bin/sh
dir=`dirname $0`;
. $dir/../exit_code.sh.inc
pip_is_debug_build;
$dir/../util/pip_is_debug_build;
#if [ $? -ne 0 ] ; then
#    exit $EXIT_UNTESTED;
#fi

ntasks=$1;
niters=100
fname=seek-outin.text
$dir/seek stdout $ntasks $niters > $fname
rv=$?
if [[ $rv -ne 0 ]]; then
    exit $rv
fi
$dir/seek stdin $ntasks niters < $fname
rv=$?
if [[ $rv -ne 0 ]]; then
    exit $rv
fi
cat $fname | $dir/seek stdin $ntasks $niters
rv=$?
rm -f $fname
exit $rv
