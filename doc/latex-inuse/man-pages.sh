#!/bin/sh

# $PIP_license: <Simplified BSD License>
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
#     Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
# 
#     Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.
# $
# $RIKEN_copyright: Riken Center for Computational Sceience (R-CCS),
# System Software Development Team, 2016-2021
# $
# $PIP_VERSION: Version 3.0.0$
#
# $Author: Atsushi Hori (R-CCS)
# Query:   procinproc-info@googlegroups.com
# User ML: procinproc-users@googlegroups.com
# $

prog=$0;

man1_input="man1-inputs.tex"
man3_pip_input="man3-pip-inputs.tex"
man3_ulp_input="man3-ulp-inputs.tex"

echo > $man1_input;
echo > $man3_pip_input;
echo > $man3_ulp_input;

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
    echo "\input{../latex/$bn}" >> $man3_pip_input;
done

man3_tex=`ls ../latex/group__ULP*.tex`
for man3 in $man3_tex; do
    bn=`basename -s.tex $man3`;
    echo "\input{../latex/$bn}" >> $man3_ulp_input;
done
