#!/bin/sh

. ../test.sh.inc

nt=`../util/dlmopen_count`
nt=`dc -e "$nt 100 * p"`

./pthread 2>&1 | test_msg_count 'pthread_join' $nt
