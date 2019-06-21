#!/bin/sh

dir=`dirname $0`
. $dir/exit_code.sh.inc

if [[ x$PIP_MODE == xpthread ]]; then
    exit $EXIT_UNSUPPORTED;
fi
exec $@;
