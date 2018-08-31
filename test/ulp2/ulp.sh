#!/bin/sh

. ../test.sh.inc

$MCEXEC ./ulp $TEST_PIP_TASKS 2>&1 | test_msg_count 'OK' 1
