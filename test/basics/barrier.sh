#!/bin/sh

. ../test.sh.inc

./barrier 2>&1 | test_msg_count 'Hello, I am fine !!'
