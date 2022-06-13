#!/bin/bash -e

if [ $# -lt 1 ] ; then
    echo "This script will lower the bitcode from ObjectIR to LLVM IR." ;
    echo "  USAGE: `basename $0` <IR_FILE> [<OPTIONS, ...>]" ;
fi

GIT_ROOT=`git rev-parse --show-toplevel` ;
LIB_DIR=${GIT_ROOT}/install/lib ;

source ${GIT_ROOT}/enable ;

OUT_DIR=$(dirname $(realpath ${IR_FILE})) ;

IR_FILE_LOWERED=${OUT_DIR}/all_in_one_lowered.bc ;
echo "Lower Objects (I: ${IR_FILE}, O: ${IR_FILE_LOWERED})" ;
noelle-load -load ${LIB_DIR}/ObjectLowering.so -ObjectLowering ${IR_FILE} -o ${IR_FILE} ;
cp ${IR_FILE} ${IR_FILE_LOWERED} ;
