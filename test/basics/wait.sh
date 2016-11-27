#!/bin/sh

. ../test.sh.inc

./wait 2>&1 | test_msg_count 'Hello, I am fine !!' 1
