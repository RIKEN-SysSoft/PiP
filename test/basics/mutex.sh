#!/bin/sh

. ../test.sh.inc

./mutex 2>&1 | test_msg_count 'Hello, I am here (counter=' 100
