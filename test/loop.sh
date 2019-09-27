#!/bin/sh

cmdline="$0 $@";
cmd=`basename $0`;
ext=0;
TMP=''

prt_ext()
{
    exit=$1
    if [ $quiet -eq 0 ]; then
	echo;
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
}

print_usage()
{
    echo >&2 "Usage: $cmd [-n <NITER>] [-t <SEC>] [-q] [<test_prog> ...]";
    echo >&2 "    -n <NITER>: Number of iterations";
    echo >&2 "    -t <SEC>: Duration limit of one loop [seconds]";
    echo >&2 "    -q: Quiet mode";
    exit 2;
}

finalize()
{
    if [ x"$TMP" != x ]; then
	echo "Logfile: $FILE";
	mv $TMP $FILE;
    fi
}

sigsegv()
{
    echo;
    echo "SIGEGV";
    finalize;
    exit 127;
}

control_c()
{
    echo;
    finalize;
    exit 4;
}

trap sigsegv 11
trap control_c 2

duration=0;
iteration=0;
quiet=0;
display=0;

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
	    case $1 in *D)        display=1;;    esac
	    case $1 in *h | *u)   print_usage;;  esac
	    shift;
        done
	;;
esac

if [ $# -lt 1 ]; then
    print_usage;
fi

if [ ! -x $1 ]; then
    echo "$1 is not executable";
    exit 5;
fi

PROGNAM=`basename $1`;
FILE="loop-$$.log";
TMP=.$FILE;

i=0;
start=`date +%s`;

while true; do
    date > $TMP;
    echo "$cmdline" >> $TMP;
    echo "---------------------------------" >> $TMP;

    if [ $quiet -eq 0 ]; then
	echo -n $i "";
    fi

    if [ $display -eq 0 ]; then
	"$@" >> $TMP 2>&1;
    else
	"$@" 2>&1 | tee -a $TMP;
    fi
    ext=$?;
    if [ $ext != 0 ]; then
	prt_ext $ext;
	finalize;
	exit $ext;
    else
	rm -f $TMP
	touch $TMP
    fi

    i=$((i+1));
    if [ $iteration -gt 0 -a $i -gt $iteration ]; then
	if [ $quiet -eq 0 ]; then
	    echo;
	    echo Repeated $iteration times;
	fi
	break;
    fi

    now=`date +%s`;
    elaps=$((now-start));
    if [ $duration -gt 0 -a $elaps -gt $duration ]; then
	if [ $quiet -eq 0 ]; then
	    echo;
	    echo Time up "($duration Sec)";
	fi
	break;
    fi
done

rm -f $TMP;

prt_ext 0;
exit 0;
