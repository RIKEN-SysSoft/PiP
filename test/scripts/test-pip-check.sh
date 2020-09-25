#!/bin/sh

tstp=$0
. ../exit_code.sh.inc

./pipcc --nopip a.c -o nopip
if ./pip-check nopip; then
    echo "./pip-check nopip -- FAILS"
    rm -f nopip
    exit $EXIT_FAIL;
else
    echo "./pip-check nopip -- SUCCESS"
fi
rm -f nopip

./pipcc --piproot a.c -o piproot
if ! ./pip-check --piproot piproot; then
    echo "! ./pip-check --piproot piproot -- FAILS"
    rm -f piproot
    exit $EXIT_FAIL;
else
    echo "! ./pip-check --piproot piproot -- SUCCESS"
fi
if ./pip-check --piptask -o piproot; then
    echo "./pip-check --piptask piproot -- FAILS"
    rm -f piproot
    exit $EXIT_FAIL;
else
    echo "./pip-check --piptask piproot -- SUCCESS"
fi
rm -f piproot

./pipcc --piptask a.c -o piptask
if ! ./pip-check --piptask piptask; then
    echo "! ./pip-check --piptask piptask -- FAILS"
    rm -f piptask
    exit $EXIT_FAIL;
else
    echo "! ./pip-check --piptask piptask -- SUCCESS"
fi
##if ./pip-check --piproot piptask; then
##    echo "./pip-check --piproot piptask -- FAILS"
##    rm -f piptask
##    exit $EXIT_FAIL;
##else
##    echo "./pip-check --piproot piptask -- SUCCESS"
##fi
rm -f piptask

./pipcc a.c -o pipboth
if ! ./pip-check --both pipboth; then
    echo "! ./pip-check --both pipboth -- FAILS"
    rm -f pipboth
    exit $EXIT_FAIL;
else
    echo "! ./pip-check --both pipboth -- SUCCESS"
fi
if ! ./pip-check --piproot pipboth; then
    echo "! ./pip-check --piproot pipboth -- FAILS"
    rm -f pipboth
    exit $EXIT_FAIL;
else
    echo "! ./pip-check --piproot pipboth -- SUCCESS"
fi
if ! ./pip-check --piptask pipboth; then
    echo "! ./pip-check --piptask pipboth -- FAILS"
    rm -f pipboth
    exit $EXIT_FAIL;
else
    echo "! ./pip-check --piptask pipboth -- SUCCESS"
fi
rm -f pipboth

echo $tstp ": PASS"; echo;
exit $EXIT_PASS;
