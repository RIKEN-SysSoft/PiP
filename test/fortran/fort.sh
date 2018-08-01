#!/bin/sh

. ../test.sh.inc

export GFORTRAN_UNBUFFERED_ALL=y

$MCEXEC ./fort 2>&1 | test_msg_count 'Hello World!'
