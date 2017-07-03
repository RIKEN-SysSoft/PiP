echo "cat << ignore-this-part >> /dev/null"

export LD_PRELOAD=`pwd`/../../preload/pip_preload.so

csv_begin() {
    FN=`basename \`pwd\``
    echo "cat << end-of-file-$1 > $FN-$1.csv"
}

csv_end() {
    echo "end-of-file-$1"
}

pip_mode_list () {
    export PIP_MODE=$1;
    ../../test/util/pip_mode 2>/dev/null;
    unset PIP_MODE
}

MODE_LIST=`pip_mode_list process:preload; pip_mode_list process:pipclone; pip_mode_list thread`

echo "==============================="
cat $0
echo "-------------------------------"
echo `date` "  " `git describe`
echo PIP_MODEs: $MODE_LIST
uname -a
echo "-------------------------------"

echo "ignore-this-part"
