#!/bin/sh

file=eval-all.dat;

doeval() {
    echo "cat << end-of-header > $1.dat"
    cd $1;
    ./run.sh;
    echo "end-of-header"
}

rotatelog() {
    rm -f $file.10 2>&1 > /dev/null
    mv -f $file.9 $file.10 2>&1 > /dev/null
    mv -f $file.8 $file.9 2>&1 > /dev/null
    mv -f $file.7 $file.8 2>&1 > /dev/null
    mv -f $file.6 $file.7 2>&1 > /dev/null
    mv -f $file.5 $file.6 2>&1 > /dev/null
    mv -f $file.4 $file.5 2>&1 > /dev/null
    mv -f $file.3 $file.4 2>&1 > /dev/null
    mv -f $file.2 $file.3 2>&1 > /dev/null
    mv -f $file.1 $file.2 2>&1 > /dev/null
    mv -f $file.0 $file.1 2>&1 > /dev/null
    mv -f $file   $file.0 2>&1 > /dev/null
}

rotatelog;

#doeval ctxsw 2>&1 | tee -a $file;
#doeval mmap  2>&1 | tee -a $file;
#doeval ptsz  2>&1 | tee -a $file;
#doeval spawn 2>&1 | tee -a $file;
doeval xpmem 2>&1 | tee -a $file;
