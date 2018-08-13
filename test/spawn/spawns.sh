#!/bin/sh

. ../test.sh.inc

trap 'rm -f $TEST_TMP; exit $EXIT_KILLED' $TEST_TRAP_SIGS

N0=$(expr $TEST_PIP_TASKS / 3 )
N1=$(expr $TEST_PIP_TASKS / 3 + 1 )

#./spawns >$TEST_TMP 2>&1
./spawns 2>&1 | tee $TEST_TMP > /dev/null
if [ $(fgrep 'Hello, I am fine !' <$TEST_TMP | wc -l) -eq $TEST_PIP_TASKS ] &&
   ( n=$(grep 'a\.out#[0-9][0-9]* -- Hello, I am fine !$' <$TEST_TMP | wc -l);
     [ $n -eq $N0 -o $n -eq $N1 ] ) &&
   ( n=$(grep 'b\.out#[0-9][0-9]* -- Hello, I am fine !$' <$TEST_TMP | wc -l);
     [ $n -eq $N0 -o $n -eq $N1 ] ) &&
   ( n=$(grep 'c\.out#[0-9][0-9]* -- Hello, I am fine !$' <$TEST_TMP | wc -l);
     [ $n -eq $N0 -o $n -eq $N1 ] )
then
	test_exit_status=$EXIT_PASS
fi
#rm -f $TEST_TMP
exit $test_exit_status
