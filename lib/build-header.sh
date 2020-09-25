#!/bin/sh

BUILD_HEADER="build.h"

build_date=`date -u`
commit_hash=`git rev-parse HEAD`
build_os=`uname -s -r -v`
ccomp=$1
if [ x"${ccomp}" == x ]; then
    ccomp=`which cc`
fi
build_cc=`$ccomp --version 2>&1 | head -n 1`
if [ x"${build_cc}" == x ]; then
    build_cc=$ccomp
fi
build_cc=`echo $build_cc`

echo -n > ${BUILD_HEADER}

echo "#define BUILD_DATE \"${build_date}\""    >> $BUILD_HEADER
echo "#define COMMIT_HASH \"${commit_hash}\""  >> $BUILD_HEADER
echo "#define BUILD_OS \"${build_os}\""        >> $BUILD_HEADER
echo "#define BUILD_CC \"${build_cc}\""        >> $BUILD_HEADER
