#!/bin/sh

. ../test.sh.inc

./fort 2>&1 | test_msg_count 'Hello World!'
