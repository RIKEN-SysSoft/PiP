#!/usr/bin/sh

file=eval-all.dat;

doeval() {
    echo "cat << EOFEOF > $1.dat"
    cd $1;
    ./run.sh;
    echo "EOFEOF"
}

rotatelog() {
    rm -f $file.10
    mv -f $file.9 $file.10
    mv -f $file.8 $file.9
    mv -f $file.7 $file.8
    mv -f $file.6 $file.7
    mv -f $file.5 $file.6
    mv -f $file.4 $file.5
    mv -f $file.3 $file.4
    mv -f $file.2 $file.3
    mv -f $file.1 $file.2
    mv -f $file.0 $file.1
    mv -f $file   $file.0
}

rotatelog;

echo "#!/usr/bin/sh" > $file

doeval ctxsw 2>&1 | tee -a $file;
doeval mmap 2>&1  | tee -a $file;
doeval ptsz 2>&1  | tee -a $file;
doeval spawn 2>&1 | tee -a $file;
doeval xpmem 2>&1 | tee -a $file;
