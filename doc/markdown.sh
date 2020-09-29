#!/bin/sh

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
