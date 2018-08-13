#!/bin/sh

. ../test.sh.inc

$MCEXEC ./exit1 2>&1 | test_msg_count 'terminated. OK' $TEST_PIP_TASKS
