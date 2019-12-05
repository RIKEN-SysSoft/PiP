#!/bin/sh
dir=`dirname $0`
$dir/../NG.sh $dir/../util/timer -1 ./inf
$dir/../NG.sh $dir/../util/timer -1 $dir/../util/pip_task 1    ./inf
$dir/../NG.sh $dir/../util/timer -1 $dir/../util/pip_task 8    ./inf
$dir/../NG.sh $dir/../util/timer -1 $dir/../util/pip_blt  1 1  ./inf
$dir/../NG.sh $dir/../util/timer -1 $dir/../util/pip_blt  8 8  ./inf
$dir/../NG.sh $dir/../util/timer -1 $dir/../util/pip_blt  8 24 ./inf
