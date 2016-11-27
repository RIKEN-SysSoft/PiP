#!/bin/sh

. ../test.sh.inc

./exit 2>&1 | test_msg_count 'pip_wait() returns 0 ???' $(expr $TEST_PIP_TASKS - 1)
