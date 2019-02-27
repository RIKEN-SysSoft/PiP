#!/bin/sh

if test -z $1; then
    echo "./add_keyword <path-to-the-expand-keyword-command>";
    exit 1;
fi

keyword=$1
$keyword -config keywords -tag copyright,version,bsd2 ../
