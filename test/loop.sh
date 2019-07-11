#!/bin/sh

cmd=`basename $0`;

function prt_ext() {
    exit=$1
    if [ $quiet -eq 0 ]
    then
	tty > /dev/null 2>&1;
	if [ $? ]
	then
	    case $exit in
		0) echo "EXIT_PASS";;
		1) echo "EXIT_FAIL";;
		2) echo "EXIT_XPASS";;
		3) echo "EXIT_XFAIL";;
		4) echo "EXIT_UNRESOLVED";;
		5) echo "EXIT_UNTESTED";;
		6) echo "EXIT_UNSUPPORTED";;
		7) echo "EXIT_KILLED";;
		*) echo "(unknown:" $1 ")";;
	    esac
	fi
    fi
    exit $exit;
}

function print_usage() {
    echo >&2 "Usage: $cmd [-n <NITER>] [-t <SEC>] [-q] [<test_prog> ...]";
    echo >&2 "    -n <NITER>: Number of iterations";
    echo >&2 "    -t <SEC>: Duration limit of one loop [seconds]";
    echo >&2 "    -q: Quiet mode";
    exit 2;
}

TMP=''

function finalize() {
    if [[ x$TMP != x ]]; then
	echo;
	echo -n "Logfile: $FILE";
	mv $TMP $FILE;
    fi
    if [ $quiet -eq 0 ]; then
	echo;
    fi
    prt_ext $ext;
}

function sigsegv() {
    echo SIGEGV;
    finalize;
}

trap sigsegv 11

duration=0;
iteration=0;
quiet=0;

case $# in
    0)	print_usage;;
    *)	while	case $1 in
	-*) true;;
         *) false;;
                esac
        do
	    case $1 in *n) shift; iteration=$1;; esac
	    case $1 in *t) shift; duration=$1;;  esac
	    case $1 in *q)        quiet=1;;      esac
	    case $1 in *h | *u)   print_usage;;  esac
	    shift;
        done
	;;
esac

if [ $# -lt 1 ]; then
    print_usage;
fi

if [ ! -x $1 ]; then
    echo "$1 is not executable"
fi

PROGNAM=`basename $1`;
FILE="loop-$$.log";
TMP=.$FILE;

i=0;
start=`date +%s`;

while [ true ]
do
    if [ $quiet -eq 0 ]
    then
	echo -n $i "";
    fi

    "$@" > $TMP 2>&1
    ext=$?;
    if [ $ext != 0 ]
    then
	finalize
    else
	truncate --size=0 $TMP;
    fi

    i=$((i+1));
    if [ $iteration -gt 0 -a $i -gt $iteration ]
    then
	if [ $quiet -eq 0 ]
	then
	    echo;
	    echo Repeated $iteration times;
	fi
	break;
    fi

    now=`date +%s`;
    elaps=$((now-start));
    if [ $duration -gt 0 -a $elaps -gt $duration ]
    then
	if [ $quiet -eq 0 ]
	then
	    echo;
	    echo Time up "($duration Sec)";
	fi
	break;
    fi
done

rm -f $TMP;

prt_ext 0;
