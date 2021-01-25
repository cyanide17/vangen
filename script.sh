#!/bin/bash

PWD=`pwd -P`
PRJ_PATH="~/Desktop/hypervisor-fuzz"
SRC_PATH="$PRJ_PATH/src"
LOG=$PWD/$1
ARGERR=0

set -e
echo "##|"

# argument check
if [ -z $1 ];then
  echo "##| [ERR ] no argument!"
  ARGERR=1
fi
if [ ! -z $3 ];then
  echo "##| [ERR ] too many arguments!"
  ARGERR=1
elif [ "$2" == "-dbg" ];then
  echo "##| [INFO] DEBUG MODE"
  echo "##|"
elif [ ! -z $2 ];then
  echo "##| [ERR ] wrong option!"
  ARGERR=1
fi
if [ $ARGERR -eq 1 ];then
  echo "##| [INFO] USAGE: $./script.sh 'relative-path-to-log-file'"
  echo "##|               or"
  echo "##|               $./script.sh 'relative-path-to-log-file' -dbg"
  echo "##|               for debugging"
  echo "##| [ERR ] TERMINATING..."
  echo "##|"
  exit 1
fi

# log file existence check
if [ ! -f $LOG ];then
    echo "##| [ERR ] log file does not exist"
    echo "##| [ERR ] TERMINATING..."
    echo "##|"
    exit 1
fi

# directory existence check
if [ ! -d "$PWD/dbg" ];then
    mkdir dbg
fi



# ===========
#   PARSING
# ===========

# binary file recompile
if [ ! -f $PWD/src/parser.c ];then
    echo "##| [ERR ] parser.c does not exist"
    echo "##| [ERR ] TERMINATING..."
    echo "##|"
    exit 1
fi
gcc $PWD/src/parser.c -o $PWD/dbg/parser -lm
# parse
echo "##| [INFO] parsing.."
$PWD/dbg/parser $LOG > $PWD/dbg/repro

# execution exit status check
if [ $? -ne 0 ];then
    echo "##| [ERR ] parse failed"
    echo "##| [ERR ] TERMINATING..."
    echo "##|"
    exit 1
fi
echo "##| [INFO] PARSE DONE."
echo "##|"



# ===========
#   MERGING
# ===========

# binary file recompile
if [ ! -f $PWD/src/merger.c ];then
    echo "##| [ERR ] merger.c does not exist"
    echo "##| [ERR ] TERMINATING..."
    echo "##|"
    exit 1
fi
gcc $PWD/src/merger.c -o $PWD/dbg/merger

# template existence check
if [ ! -f $PWD/src/template.c ];then
    echo "##| [ERR ] template.c does not exist"
    echo "##| [ERR ] TERMINATING..."
    exit 1
fi
cp $PWD/src/template.c $PWD/dbg/template

# merge
echo "##| [INFO] merging.."
$PWD/dbg/merger $PWD/dbg/template $PWD/dbg/repro $PWD/dbg/mod.c

# execution exit status check
if [ $? -ne 0 ];then
    echo "##| [ERR ] merge failed"
    echo "##| [ERR ] TERMINATING..."
    exit 1
fi
echo "##| [INFO] MERGE DONE."
echo "##|"



# ===========
#  compiling
# ===========

# Makefile existence check
if [ ! -f $PWD/src/Makefile ];then
    echo "##| [ERR ] Makefile does not exist"
    echo "##| [ERR ] TERMINATING..."
    exit 1
fi
cp $PWD/src/Makefile $PWD/dbg
cd $PWD/dbg

# compile
echo "##| [INFO] compiling.."
make clean > /dev/null
make > /dev/null

# execution exit status check
if [ $? -ne 0 ];then
    echo "##| [ERR ] compile failed (make)"
    echo "##| [ERR ] TERMINATING..."
    exit 1
fi

# remove files except mod.ko
mv $PWD/mod.ko $PWD/..
make clean > /dev/null
cd $PWD/..
if [ -z $2 ];then
  rm dbg -rf
fi

scp -P2345 -i $SRC_PATH/image/stretch.id_rsa mod.ko root@localhost:~/ > /dev/null
if [ -z $2 ];then
  rm mod.ko
fi

# execution exit status check
if [ $? -ne 0 ];then
    echo "##| [ERR ] compile failed"
    echo "##| [ERR ] TERMINATING..."
    exit 1
fi
echo "##| [INFO] COMPILE DONE."
echo "##|"

