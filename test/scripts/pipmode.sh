#!/bin/sh

dir=`dirname $0`
. $dir/../exit_code.sh.inc

testdir=$dir/../..

out=`./pipmode -p ../util/pip_mode`;
if [ "$out" != "process:preload" ]; then
    echo "./pipmode -p ../util/pip_mode" "FAILS"
    echo "Output: $out"
    exit $EXIT_FAIL;
fi

out=`./pipmode -c ../util/pip_mode`;
if [ "$out" != "process:pipclone" ]; then
    echo "./pipmode -c ../util/pip_mode" "FAILS"
    echo "Output: $out"
    exit $EXIT_FAIL;
fi

out=`./pipmode -t ../util/pip_mode`;
if [ "$out" != "pthread" ]; then
    echo "./pipmode -t ../util/pip_mode" "FAILS"
    echo "Output: $out"
    exit $EXIT_FAIL;
fi

exit $EXIT_PASSD;
