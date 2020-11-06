#!/bin/sh

prog=`basename $0`

if test -z $1; then
    echo "${prog} <path-to-the-expand-keyword-command>";
    exit 1;
fi

keyword=$1
exec $keyword -config keywords,version -tag copyright,version,bsd2 ../
