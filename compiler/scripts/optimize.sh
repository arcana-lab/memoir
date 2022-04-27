#!/bin/bash -e

GIT_ROOT=`git rev-parse --show-toplevel` ;
COMPILER_DIR=${GIT_ROOT}/compiler ;

IR_FILE="$1" ;

opt -mem2reg -o ${IR_FILE} ${IR_FILE}
noelle-norm ${IR_FILE} -o ${IR_FILE};

echo "Lower Objects (I: ${IR_FILE}, O: ${IR_FILE})" ;
IR_FILE_LOWERED=all_in_one_lowered.bc ;
noelle-load -load ${COMPILER_DIR}/passes/build/lib/ObjectLowering.so -ObjectLowering ${IR_FILE} -o ${IR_FILE} ;
cp ${IR_FILE} ${IR_FILE_LOWERED} ;
