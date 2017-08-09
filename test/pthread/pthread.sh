#!/bin/sh

. ../test.sh.inc

nt=`../util/dlmopen_count`
nt=`dc -e "$nt 100 * p"`

$MCEXEC ./pthread 2>&1 | test_msg_count 'SUCCESS' $nt
