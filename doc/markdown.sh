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
# Query:   procinproc-info+noreply@googlegroups.com
# User ML: procinproc-users+noreply@googlegroups.com
# $

title=$1;
sec=$2;
prefix=$3;

items=`ls man/man$sec/*.$sec`;

for item in $items; do
    bn=`basename -s.$sec $item`
    case $bn in
	md_*)  true;;
	PiP-*) true;;
	ULP-*) true;;
	*) list="$list $bn";;
    esac
done

echo;
echo $title;
for item in $list; do
    it=${item//_/\\_};
    echo $prefix $it;
done
echo;

exit 0;
