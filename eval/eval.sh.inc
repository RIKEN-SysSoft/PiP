echo "cat << ignore-this-part >> /dev/null"

export LD_PRELOAD=`pwd`/../../preload/pip_preload.so
export LD_LIBRARY_PATH=`pwd`/../../lib/:$LD_LIBRARY_PATH

echo "LD_PRELOAD=     " $LD_PRELOAD
echo "LD_LIBRARY_PATH=" $LD_LIBRARY_PATH

csv_begin() {
    FN=`basename \`pwd\``
    echo "cat << end-of-file-$1 > $FN-$1.csv"
}

csv_end() {
    echo "end-of-file-$1"
}

pip_mode_list() {
    export PIP_MODE=$1;
    ../../test/util/pip_mode 2>/dev/null;
    unset PIP_MODE
}

NUMACTL() {
if [ -n "$MCEXEC" ]
then
	shift;
	$MCEXEC $@;
else
    CORENO=$1;
    shift;
    ../util/corebind $CORENO $@;
fi
}

CORENUM=`$MCEXEC getconf _NPROCESSORS_ONLN`

if [ -n "$MCEXEC" ]
then
    MODE_LIST=pthread;
    NTMAX=$CORENUM
else
    MODE_LIST=`pip_mode_list process:preload; \
	   pip_mode_list process:pipclone; \
   	   pip_mode_list thread;`;
    NTMAX=`../../test/util/dlmopen_count`
fi

if [ -z $ITER_NUM ]
then
    ITER_NUM="1 2 3 4 5 6 7 8 9 10 11 12";
fi

echo "==============================="
../util/commentize $0
echo "-------------------------------"
uname -a
echo "PageSize:  " `getconf PAGESIZE`
echo "#CPU Cores:" $CORENUM
grep -e "$ ./configure --" ../../config.log
echo `date` "  " `git describe`
echo "PIP_MODEs:" $MODE_LIST
echo "NTMAX:    " $NTMAX
echo "ITER_NUM: " $ITER_NUM
if [ -n "$MCEXEC" ]
then
    echo mcexec: $MCEXEC
fi
echo "-------------------------------"

echo "ignore-this-part"
