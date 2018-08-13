#!/bin/sh

. ../test.sh.inc

$MCEXEC ./namexp 2>&1 | test_msg_count 'success' 1
