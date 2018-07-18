#!/bin/sh

. ../test.sh.inc

$MCEXEC ./sigchld2 2>&1 | test_msg_count 'SUCCEEDED' 1
