#!/bin/bash -e

if [ $# -lt 1 ] ; then
    echo "This script will lower the bitcode from MemOIR to LLVM IR." ;
    echo "  USAGE: `basename $0` <INPUT IR FILE> <OUTPUT IR FILE>  [<OPTIONS, ...>]" ;
fi

GIT_ROOT=`git rev-parse --show-toplevel` ;
LIB_DIR=${GIT_ROOT}/install/lib ;

source ${GIT_ROOT}/enable ;

IR_FILE_IN="$1" ;
IR_FILE_OUT="$2" ;

echo "" ;
echo "Lowering pipeline (I: ${IR_FILE_IN}, O: ${IR_FILE_OUT})" ;

IR_FILE_BASENAME=$(basename -- ${IR_FILE_IN}) ;
IR_FILE_FILENAME="${IR_FILE_BASENAME%.*}" ;

OUT_DIR=$(dirname ${IR_FILE_IN}) ;

IR_FILE=${OUT_DIR}/${IR_FILE_FILENAME}_temp.bc ;
cp ${IR_FILE_IN} ${IR_FILE} ;

IR_FILE_LOWERED=${OUT_DIR}/${IR_FILE_FILENAME}_lowered.bc ;
echo "Lower Objects (I: ${IR_FILE}, O: ${IR_FILE_LOWERED})" ;
noelle-load -load ${LIB_DIR}/ObjectLowering.so -ObjectLowering ${IR_FILE} -o ${IR_FILE} ;
cp ${IR_FILE} ${IR_FILE_LOWERED} ;

cp ${IR_FILE} ${IR_FILE_OUT} ;
