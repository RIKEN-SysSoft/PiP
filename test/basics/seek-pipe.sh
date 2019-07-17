#!/bin/sh
dir=`dirname $0`
ntasks=$1;
niters=100
$dir/seek stdout $1 100 | $dir/seek stdin $ntasks $niters
exit $?
