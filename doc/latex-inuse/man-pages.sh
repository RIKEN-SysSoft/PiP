#!/bin/sh

prog=$0;

man1_input="man1-inputs.tex"
man3_pip_input="man3-pip-inputs.tex"
man3_ulp_input="man3-ulp-inputs.tex"
man3_input="man3-inputs.tex"

echo > $man1_input;
echo > $man3_pip_input;
echo > $man3_ulp_input;
echo > $man3_input;

# man7
cp ../latex/libpip.tex .

man1_tex=`ls ../man/man1/*.1`
for man1 in $man1_tex; do
    bn=`basename -s.1 $man1`;
    if [ -f ../latex/$bn.tex ]; then
	tex=${bn//_/\\_};
	echo "\section{$tex}" >> $man1_input;
	echo "\input{../latex/$bn}" >> $man1_input;
    elif [ -f ../latex/group__$bn.tex ]; then
	tex=${bn//_/\\_};
	echo "\input{../latex/group__$bn}" >> $man1_input;
    else
	echo "$prog: $bn not found"
    fi
done

man3_tex=`ls ../latex/group__PiP*.tex`
for man3 in $man3_tex; do
    bn=`basename -s.tex $man3`;
    echo "\input{../latex/$bn}" >> $man3_input;
done
