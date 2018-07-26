#!/bin/sh

. ../test.sh.inc

$MCEXEC ./mutex $TEST_PIP_TASKS 2>&1 | test_msg_count 'Succeeded' 1
