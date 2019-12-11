#!/bin/sh

tstp=$0
dir=`dirname $0`
pipdir=$dir/../..
. $dir/../exit_code.sh.inc

$dir/pipcc --pipdir $pipdir --nopip a.c
if $dir/pip-check a.out; then
    exit $EXIT_FAIL;
fi

$dir/pipcc --pipdir $pipdir --piproot a.c
if ! $dir/pip-check --piproot a.out; then
    exit $EXIT_FAIL;
fi
if $dir/pip-check --piptask a.out; then
    exit $EXIT_FAIL;
fi

$dir/pipcc --pipdir $pipdir --piptask a.c
if ! $dir/pip-check --piptask a.out; then
    exit $EXIT_FAIL;
fi
if $dir/pip-check --piproot a.out; then
    exit $EXIT_FAIL;
fi

$dir/pipcc --pipdir $pipdir a.c
if ! $dir/pip-check --both a.out; then
    exit $EXIT_FAIL;
fi
if ! $dir/pip-check --piproot a.out; then
    exit $EXIT_FAIL;
fi
if ! $dir/pip-check --piptask a.out; then
    exit $EXIT_FAIL;
fi

echo $tstp ": PASS"; echo;
exit $EXIT_PASS;
