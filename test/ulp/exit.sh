#!/bin/sh

. ../test.sh.inc

##unset TEST_PIP_TASKS
##TEST_PIP_TASKS=`../util/dlmopen_count -p -m 250`

##echo TEST_PIP_TASKS=$TEST_PIP_TASKS

$MCEXEC ./exit $TEST_PIP_TASKS 2>&1 | test_msg_count 'Succeeded (' $TEST_PIP_TASKS
