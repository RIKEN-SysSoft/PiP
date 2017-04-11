#!/bin/sh

. ../test.sh.inc

# PIP_MODE_PROCESS is specified in pip_init()
case `../util/pip_mode` in
pthread) exit $EXIT_UNSUPPORTED;;
esac

./file 2>&1 | test_msg_count 'checking -- ok' $(expr $TEST_PIP_TASKS + 1)
