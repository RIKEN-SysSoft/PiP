#!/bin/sh

. ../test.sh.inc

$MCEXEC ./sigchld $TEST_PIP_TASKS 2>&1 | test_msg_count 'SUCCEEDED' 1
