#!/bin/sh

prog=$0;

if [ ! -d html ] ; then
    if ! mkdir html;           then exit 1; fi
    if ! mkdir html/commands;  then exit 1; fi
    if ! mkdir html/functions; then exit 1; fi
else
    if [ ! -d html/commands ] ;  then
	if ! mkdir html/commands;  then exit 1; fi
    fi
    if [ ! -d html/functions ] ; then
	if ! mkdir html/functions; then exit 1; fi
    fi
fi

man1_tex=`ls ../man/man1/*.1`
for man1 in $man1_tex; do
    bn=`basename -s.1 $man1`;
    if [ -f ../latex/$bn.tex ]; then
	tex=${bn//_/\\_};
	echo "\section{$tex}" >> $man1_input;
	echo "\input{../latex/$bn}" >> $man1_input;
    else
	echo "$prog: $bn not found"
    fi
done

man3_tex=`ls ../man/man3/*.3`
for man3 in $man3_tex; do
    bn=`basename -s.3 $man3`;
    if [ -f ../latex/$bn.tex ]; then
	tex=${bn//_/\\_};
	echo "\section{$tex}" >> $man3_input;
	echo "\input{../latex/$bn}" >> $man3_input;
    elif [ -f ../latex/group__$bn.tex ]; then
	tex=${bn//_/\\_};
	case $bn in
	    pip*) echo "\input{../latex/group__$bn}" >> $man3_pip_input;;
	    ulp*) echo "\input{../latex/group__$bn}" >> $man3_ulp_input;;
	    *)    echo "\input{../latex/group__$bn}" >> $man3_input;;
	esac
    else
	echo "$prog: $bn not found"
    fi
done
