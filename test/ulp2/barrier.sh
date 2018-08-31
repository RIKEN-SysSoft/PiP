#!/bin/sh

. ../test.sh.inc

$MCEXEC ./barrier $TEST_PIP_TASKS 2>&1 | test_msg_count 'Good' 1
