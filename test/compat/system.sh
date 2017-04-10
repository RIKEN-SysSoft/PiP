#!/bin/sh

. ../test.sh.inc

hostname=`hostname`

./spawn $TEST_PIP_TASKS 2>&1 | test_msg_count $hostname $TEST_PIP_TASKS
