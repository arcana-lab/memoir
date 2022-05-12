#!/bin/bash -e

GIT_ROOT=`git rev-parse --show-toplevel` ;
LIB_DIR=${GIT_ROOT}/install/lib ;

source ${GIT_ROOT}/enable ;

IR_FILE="$1" ;

OUT_DIR=$(dirname $(realpath ${IR_FILE}));

IR_FILE_NORM=${OUT_DIR}/all_in_one_norm.bc ;
echo "Normalize Bitcode (I: ${IR_FILE}, O: ${IR_FILE_NORM})" ;
noelle-norm ${IR_FILE} -o ${IR_FILE};
cp ${IR_FILE} ${IR_FILE_NORM} ;

IR_FILE_LOWERED=${OUT_DIR}/all_in_one_lowered.bc ;
echo "Lower Objects (I: ${IR_FILE}, O: ${IR_FILE_LOWERED})" ;
noelle-load -load ${LIB_DIR}/ObjectLowering.so -ObjectLowering ${IR_FILE} -o ${IR_FILE} ;
cp ${IR_FILE} ${IR_FILE_LOWERED} ;
