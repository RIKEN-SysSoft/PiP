#!/bin/sh

dir=`dirname $0`
. $dir/exit_code.sh.inc

if [[ x$PIP_MODE == x$1 ]]; then
    exit $EXIT_UNSUPPORTED;
fi
shift;
exec $@;
