echo "cat << ignore-this-part >> /dev/null"

export LD_PRELOAD=`pwd`/../../preload/pip_preload.so
export LD_LIBRARY_PATH=`pwd`/../../lib/:$LD_LIBRARY_PATH

echo $LD_PRELOAD
echo $LD_LIBRARY_PATH

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
    CORENO=$1;
    shift;
    ../util/corebind $CORENO $@;
}

MODE_LIST=`pip_mode_list process:preload; \
	   pip_mode_list process:pipclone; \
	   pip_mode_list thread`

echo "==============================="
../util/commentize $0
echo "-------------------------------"
uname -a
echo "PageSize:  " `getconf PAGESIZE`
echo "#CPU Cores:" `getconf _NPROCESSORS_ONLN`
grep -e "$ ./configure --" ../../config.log
echo `date` "  " `git describe`
echo PIP_MODEs: $MODE_LIST
echo "-------------------------------"

echo "ignore-this-part"
