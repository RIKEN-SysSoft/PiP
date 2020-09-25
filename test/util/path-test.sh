#!/bin/sh

PROG_RPATH=../prog/abc
PROG_APATH=`realpath ../prog/abc`

echo "relative path:" $PROG_RPATH
echo "absolute path:" $PROG_APATH

./pip_task 1 $PROG_RPATH
ev=$?
if [ $ev != 0 ]; then
    exit $ev;
fi
./pip_task 8 $PROG_RPATH
ev=$?
if [ $ev != 0 ]; then
    exit $ev;
fi

./pip_task 1 $PROG_APATH
ev=$?
if [ $ev != 0 ]; then
    exit $ev;
fi
./pip_task 8 $PROG_APATH
ev=$?
if [ $ev != 0 ]; then
    exit $ev;
fi

exit 0;
