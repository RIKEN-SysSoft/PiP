#!/bin/sh
dir=`dirname $0`
ntasks=$1;
niters=100
fname=seek-file.text
$dir/seek $fname $ntasks $niters
rv=$?
rm -f $fname
exit $rv
