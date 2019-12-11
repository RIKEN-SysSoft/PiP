#!/bin/sh

tstp=$0
dir=`dirname $0`
. $dir/../exit_code.sh.inc

testdir=$dir/../..

out=`$dir/pip-mode -p $dir/../../bin/printpipmode`;
if [ "$out" != "process:preload" ]; then
    echo "./pipmode -p ../util/pip_mode" "FAILS"
    echo "Output: $out"
    exit $EXIT_FAIL;
fi

out=`$dir/pip-mode -c $dir/../../bin/printpipmode`;
if [ "$out" != "process:pipclone" ]; then
    echo "./pipmode -c ../util/pip_mode" "FAILS"
    echo "Output: $out"
    exit $EXIT_FAIL;
fi

out=`$dir/pip-mode -t $dir/../../bin/printpipmode`;
if [ "$out" != "pthread" ]; then
    echo "./pipmode -t ../util/pip_mode" "FAILS"
    echo "Output: $out"
    exit $EXIT_FAIL;
fi

echo $tstp ": PASS"; echo;
exit $EXIT_PASSD;
