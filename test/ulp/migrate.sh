#!/bin/sh

. ../test.sh.inc

##echo TEST_PIP_TASKS=$TEST_PIP_TASKS

$MCEXEC ./migrate 2>&1 | test_msg_count 'SUCCEEDED' 1
