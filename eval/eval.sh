#!/bin/sh

export ITER_NUM=$1

file=eval-all.dat;
tmpf=${file}-tmp;

doeval() {
    echo "cat << end-of-an-evaluation > $1.dat"
    cd $1;
    ./run.sh;
    echo "end-of-an-evaluation"
}

rotatelog() {
    rm -f $file.10         > /dev/null 2>&1;
    mv -f $file.9 $file.10 > /dev/null 2>&1;
    mv -f $file.8 $file.9  > /dev/null 2>&1;
    mv -f $file.7 $file.8  > /dev/null 2>&1;
    mv -f $file.6 $file.7  > /dev/null 2>&1;
    mv -f $file.5 $file.6  > /dev/null 2>&1;
    mv -f $file.4 $file.5  > /dev/null 2>&1;
    mv -f $file.3 $file.4  > /dev/null 2>&1;
    mv -f $file.2 $file.3  > /dev/null 2>&1;
    mv -f $file.1 $file.2  > /dev/null 2>&1;
    mv -f $file.0 $file.1  > /dev/null 2>&1;
    mv -f $file   $file.0  > /dev/null 2>&1;
}

rm -f $tmpf;

doeval ctxsw 2>&1 | tee -a $tmpf;
doeval mmap  2>&1 | tee -a $tmpf;
doeval ptsz  2>&1 | tee -a $tmpf;
doeval spawn 2>&1 | tee -a $tmpf;
doeval xpmem 2>&1 | tee -a $tmpf;

rotatelog;
mv $tmpf $file;
