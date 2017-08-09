#!/bin/sh

. ../test.sh.inc

nth=`../util/ompnumthread`

$MCEXEC ./openmp 2>&1 | test_msg_count 'Hello World from thread' $nth
