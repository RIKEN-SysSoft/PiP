#!/bin/sh

. ../test.sh.inc

$MCEXEC ./ulpnew $TEST_PIP_TASKS 2>&1 | test_msg_count 'Hello, ' $TEST_PIP_TASKS