#!/bin/sh

. ../test.sh.inc

$MCEXEC ./spawn $TEST_PIP_TASKS 2>&1 | test_msg_count 'Hello, I am fine !!' $TEST_PIP_TASKS
