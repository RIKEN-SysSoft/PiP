#!/bin/sh

. ../test.sh.inc

$MCEXEC ./signal 2>/dev/null | test_msg_count 'OK' 1
