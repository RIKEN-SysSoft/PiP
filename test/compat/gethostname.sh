#!/bin/sh

. ../test.sh.inc

$MCEXEC ./gethostname 2>&1 | test_msg_count "hostname:$(hostname)" 3
