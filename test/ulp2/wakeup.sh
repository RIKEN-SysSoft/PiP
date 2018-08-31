#!/bin/sh

. ../test.sh.inc

$MCEXEC ./tls 2>&1 | test_msg_count 'OK' 1
