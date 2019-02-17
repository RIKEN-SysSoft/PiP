#!/bin/sh

FILE="loop.log"

while [ true ]
do
    $@ > $FILE 2>&1;
    if [ $? != 0 ]
    then
	killall $1;
	exit $?;
    else
	truncate --size=0 $FILE;
    fi
done
