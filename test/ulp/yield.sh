#!/bin/sh

. ../test.sh.inc

$MCEXEC ./yield 2>&1 | test_msg_count 'Hello, this is ' $TEST_PIP_TASKS
