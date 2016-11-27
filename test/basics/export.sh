#!/bin/sh

. ../test.sh.inc

./export 2>&1 | test_msg_count 'Hello, my PIPID is '
