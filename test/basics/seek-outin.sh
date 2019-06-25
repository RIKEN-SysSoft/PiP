#!/bin/sh
dir=`dirname $0`
fname=seek.text
$dir/seek stdout $1 100 > $fname
rv=$?
if [[ $rv -ne 0 ]]; then
    exit $rv
fi
$dir/seek stdin $1 100 < $fname
rv=$?
if [[ $rv -ne 0 ]]; then
    exit $rv
fi
cat $fname | $dir/seek stdin $1 100
rv=$?
rm -f $fname
exit $rv
