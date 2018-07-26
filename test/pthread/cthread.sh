#!/bin/sh

. ../test.sh.inc

$MCEXEC ./cthread 2>&1 | test_msg_count 'SUCCESS' $TEST_PIP_TASKS
