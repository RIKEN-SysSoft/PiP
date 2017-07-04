echo "cat << ignore-this-part >> /dev/null"

export LD_PRELOAD=`pwd`/../../preload/pip_preload.so:$LD_PRELOAD
export LD_LIBRARY_PATH=`pwd`/../../lib/:$LD_LIBRARY_PATH

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

NUMACTL_COMMAND=`which --skip-alias --skip-function numactl 2>/dev/null`;

if [ -z $NUMACTL_COMMAND ]
   then
      echo "#WARN# No 'numactl' command #WARN#"
fi

NUMACTL() {
    if [ -z $NUMACTL_COMMAND ]
    then
        shift;
	$@;
    else
        CORENO=$1;
	shift;
        $NUMACTL_COMMAND -C $CORENO $@;
    fi
}

MODE_LIST=`pip_mode_list process:preload; pip_mode_list process:pipclone; pip_mode_list thread`

echo "==============================="
../util/commentize $0
echo "-------------------------------"
grep -e "$ ./configure --" ../../config.log
echo `date` "  " `git describe`
echo PIP_MODEs: $MODE_LIST
uname -a
echo "-------------------------------"

echo "ignore-this-part"
