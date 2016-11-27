#!/bin/sh

. ../test.sh.inc

./gethostname 2>&1 | test_msg_count "hostname:$(hostname)" 3
