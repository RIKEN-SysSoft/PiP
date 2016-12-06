#!/bin/sh

. ../test.sh.inc

./exit 2>&1 | test_msg_count 'terminated. OK' $TEST_PIP_TASKS
