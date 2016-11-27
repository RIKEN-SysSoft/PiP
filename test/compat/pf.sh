#!/bin/sh

. ../test.sh.inc

./pf 2>&1 | test_msg_count 'done' 11
