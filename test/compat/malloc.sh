#!/bin/sh

. ../test.sh.inc

$MCEXEC ./malloc 2>&1 | test_msg_count 'Hello, I am fine (sz:'
