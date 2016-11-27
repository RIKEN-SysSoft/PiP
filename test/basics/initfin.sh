#!/bin/sh

. ../test.sh.inc

./initfin 2>&1 | test_msg_count 'Hello, I am just fine !!' 1
