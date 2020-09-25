#!/bin/sh

if test -z $1; then
    echo "./expand_keyword <path-to-the-expand-keyword-command>";
    exit 1;
fi

keyword=$1
$keyword -config keywords,version -tag copyright,version,bsd2 ../
