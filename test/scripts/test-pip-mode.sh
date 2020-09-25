#!/bin/sh

tstp=$0
. ../exit_code.sh.inc

out=`./pip-mode -p ../../bin/printpipmode`;
if [ "$out" != "process:preload" ]; then
    echo "./pip-mode -p ../../bin/printpipmode" "FAILS"
    echo "Output: $out"
    exit $EXIT_FAIL;
fi

##out=`./pip-mode -c ../../bin/printpipmode`;
##if [ "$out" != "process:pipclone" ]; then
##    echo "./pip-mode -c ../../bin/printpipmode" "FAILS"
##    echo "Output: $out"
##    exit $EXIT_FAIL;
##fi

out=`./pip-mode -g ../../bin/printpipmode`;
if [ "$out" != "process:got" ]; then
    echo "./pip-mode -c ../../bin/printpipmode" "FAILS"
    echo "Output: $out"
    exit $EXIT_FAIL;
fi

out=`./pip-mode -t ../../bin/printpipmode`;
if [ "$out" != "pthread" ]; then
    echo "./pip-mode -t ../../bin/printpipmode" "FAILS"
    echo "Output: $out"
    exit $EXIT_FAIL;
fi

echo $tstp ": PASS"; echo;
exit $EXIT_PASSD;
