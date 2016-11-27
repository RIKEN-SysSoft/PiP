#!/bin/sh

. ../test.sh.inc

./file 2>&1 | test_msg_count 'checking -- ok' $(expr $TEST_PIP_TASKS + 1)
