#!/bin/sh
echo '[0]' $1;
exec ../ACT.sh $1 ./barrier 10
