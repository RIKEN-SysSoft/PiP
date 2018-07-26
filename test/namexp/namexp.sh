#!/bin/sh

. ../test.sh.inc

$MCEXEC ./namexp $TEST_PIP_TASKS 2>&1 | test_msg_count 'success' 1
