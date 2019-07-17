#!/bin/sh
dir=`dirname $0`;
ntasks=$1;
niters=100
fname=seek.text
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
