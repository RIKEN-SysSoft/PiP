#!/bin/sh

. ../test.sh.inc

$MCEXEC ./barrier 2>&1 | test_msg_count 'Hello ' $TEST_PIP_TASKS
