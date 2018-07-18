#!/bin/sh

. ../test.sh.inc

for i in 1 2 3 4 5 6 7 8 9 10
do
    $MCEXEC ./waitany2 2>&1 | test_msg_count 'SUCCEEDED' 1;
    exit_code=$?;
    if [ $exit_code -ne 0 ]
    then
#	echo 'Faile ' $exit_code;
	exit $exit_code;
#    else
#	echo 'Succeeds [' $i ']';
    fi
done
