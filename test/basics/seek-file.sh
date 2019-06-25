#!/bin/sh
dir=`dirname $0`
fname=seek.text
$dir/seek $fname $1 100
rv=$?
rm -f $fname
exit $rv
