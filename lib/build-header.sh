#!/bin/sh

cc=$1

build_date=`date -u`
commit_hash=`git rev-parse HEAD`
build_os=`uname -s -r -v`

if [ "x$cc" == "x" ]; then
    cc=`/bin/which gcc 2> /dev/null`
else
    cc=`/bin/which ${cc} 2> /dev/null`
fi
if [ $? != 0 ]; then
    echo "Unable to find a C compiler" > /dev/stderr
    exit 1
fi
build_cc=`${cc} --version 2>&1 | head -n 1`
if [ x"${build_cc}" == x ]; then
    build_cc=${cc}
fi

echo "#define BUILD_DATE \"${build_date}\""
echo "#define COMMIT_HASH \"${commit_hash}\""
echo "#define BUILD_OS \"${build_os}\""
echo "#define BUILD_CC \"${build_cc}\""
