#!/bin/bash -e

GIT_ROOT=`git rev-parse --show-toplevel` ;
COMPILER_DIR=${GIT_ROOT}/compiler ;

source ${GIT_ROOT}/enable ;

IR_FILE="$1" ;

OUT_DIR=$(dirname $(realpath ${IR_FILE})) ;

IR_FILE_NORM=${OUT_DIR}/all_in_one_norm.bc ;
echo "Normalize Bitcode (I: ${IR_FILE}, O: ${IR_FILE_NORM})" ;
noelle-norm ${IR_FILE} -o ${IR_FILE};
cp ${IR_FILE} ${IR_FILE_NORM} ;

PROF_FILE="toProfileBinary" ;
IR_FILE_PROF=all_in_one_prof.bc ;
echo "Profile Bitcode (I: ${IR_FILE}, O: ${IR_FILE_PROF})" ;
noelle-prof-coverage ${IR_FILE} ${PROF_FILE} -lm -lstdc++ ;
./${PROF_FILE} ;
noelle-meta-prof-embed default.profraw ${IR_FILE} -o ${IR_FILE};
cp ${IR_FILE} ${IR_FILE_PROF} ;

IR_FILE_LOWERED=${OUT_DIR}/all_in_one_lowered.bc ;
echo "Lower Objects (I: ${IR_FILE}, O: ${IR_FILE_LOWERED})" ;
noelle-load -load ${COMPILER_DIR}/passes/build/lib/ObjectLowering.so -ObjectLowering ${IR_FILE} -o ${IR_FILE} ;
cp ${IR_FILE} ${IR_FILE_LOWERED} ;
