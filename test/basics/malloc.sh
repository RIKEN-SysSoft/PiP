#!/bin/sh

. ../test.sh.inc

export PIP_STACKSZ=1M

$MCEXEC ./malloc 2>&1 | test_msg_count 'Hello, I am fine (sz:'
