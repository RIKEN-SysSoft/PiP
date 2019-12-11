#!/bin/sh

tstp=$0
dir=`dirname $0`;
. $dir/../exit_code.sh.inc

check() {
    printf "%-60.60s %s" "Testing: $@" "==>";
    $@ > /dev/null 2>&1;
    if [ $? -eq 0 ]; then 
	echo " OK";
    else
	echo " FAIL";
	exit $EXIT_FAIL;
    fi
}

check "$dir/../NG.sh $dir/timer -1 $dir/inf";
check "$dir/../NG.sh $dir/timer -1 $dir/pip_task 1    $dir/inf";
check "$dir/../NG.sh $dir/timer -1 $dir/pip_task 8    $dir/inf";
check "$dir/../NG.sh $dir/timer -1 $dir/pip_blt  1 1  $dir/inf";
check "$dir/../NG.sh $dir/timer -1 $dir/pip_blt  8 8  $dir/inf";
check "$dir/../NG.sh $dir/timer -1 $dir/pip_blt  8 24 $dir/inf";

echo $tstp ": PASS"; echo;
exit $EXIT_PASS;
