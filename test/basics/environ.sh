#!/bin/sh

. ../test.sh.inc

$MCEXEC ./environ 2>&1 | test_msg_count 'Hello, I am very fine !!'
