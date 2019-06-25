#!/bin/sh
dir=`dirname $0`
$dir/seek stdout $1 100 | $dir/seek stdin $1 100
exit $?
