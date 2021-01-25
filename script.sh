#!/bin/bash

set -e
echo "##|"
if [ -z $1 ];then
    echo "##| [ERR ] no argument!"
    echo "##| [INFO] USAGE: $./script.sh 'relative-path-to-log-file'"
    echo "##| [ERR ] TERMINATING..."
    echo "##|"
    exit 1
fi

PWD=`pwd -P`
LOG=$PWD/$1

# log file existence check
if [ ! -f $LOG ];then
    echo "##| [ERR ] log file does not exist"
    echo "##| [ERR ] TERMINATING..."
    echo "##|"
    exit 1
fi

# directory existence check
if [ ! -d "$PWD/parseroom" ];then
    mkdir parseroom
fi
if [ ! -d "$PWD/mergeroom" ];then
    mkdir mergeroom
fi
if [ ! -d "$PWD/compileroom" ];then
    mkdir compileroom
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
gcc $PWD/src/parser.c -o $PWD/parseroom/parser -lm
# parse
echo "##| [INFO] parsing.."
$PWD/parseroom/parser $LOG > $PWD/mergeroom/repro

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
gcc $PWD/src/merger.c -o $PWD/mergeroom/merger

# template existence check
if [ ! -f $PWD/src/template.c ];then
    echo "##| [ERR ] template.c does not exist"
    echo "##| [ERR ] TERMINATING..."
    exit 1
fi
cp $PWD/src/template.c $PWD/mergeroom/template

# merge
echo "##| [INFO] merging.."
$PWD/mergeroom/merger $PWD/mergeroom/template $PWD/mergeroom/repro $PWD/compileroom/mod.c

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
cp $PWD/src/Makefile $PWD/compileroom/
cd $PWD/compileroom/

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
scp -P2345 -i ~/Desktop/hypervisor-fuzz/src/image/stretch.id_rsa mod.ko root@localhost:~/ > /dev/null

# execution exit status check
if [ $? -ne 0 ];then
    echo "##| [ERR ] compile failed (scp)"
    echo "##| [ERR ] TERMINATING..."
    exit 1
fi
make clean > /dev/null


# execution exit status check
if [ $? -ne 0 ];then
    echo "##| [ERR ] compile failed"
    echo "##| [ERR ] TERMINATING..."
    exit 1
fi
echo "##| [INFO] COMPILE DONE."
echo "##|"


