#!/bin/sh

. ../test.sh.inc

./dlopen 2>&1 | test_msg_count 'MAIN function is at ' 1
