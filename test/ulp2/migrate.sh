#!/bin/sh

. ../test.sh.inc

$MCEXEC ./migrate 2>&1 | test_msg_count 'SUCCEEDED' 1