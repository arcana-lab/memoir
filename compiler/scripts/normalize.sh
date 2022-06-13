#!/bin/bash -e

if [ $# -lt 1 ] ; then
    echo "This script will normalize the bitcode for usage in ObjectIR and NOELLE passes." ;
    echo "  USAGE: `basename $0` <IR_FILE> [<OPTIONS, ...>]" ;
fi

GIT_ROOT=`git rev-parse --show-toplevel` ;
LIB_DIR=${GIT_ROOT}/install/lib ;

source ${GIT_ROOT}/enable ;

IR_FILE="$1" ;

OUT_DIR=$(dirname $(realpath ${IR_FILE})) ;

IR_FILE_NORM=${OUT_DIR}/all_in_one_norm.bc ;
echo "Normalize Bitcode (I: ${IR_FILE}, O: ${IR_FILE_NORM})" ;
noelle-norm ${IR_FILE} -o ${IR_FILE};
cp ${IR_FILE} ${IR_FILE_NORM} ;

PROF_FILE="toProfileBinary" ;
IR_FILE_PROF=${OUT_DIR}/all_in_one_prof.bc ;
echo "Profile Bitcode (I: ${IR_FILE}, O: ${IR_FILE_PROF})" ;
noelle-prof-coverage ${IR_FILE} ${PROF_FILE} -lm -lstdc++ ;
./${PROF_FILE} ;
noelle-meta-prof-embed default.profraw ${IR_FILE} -o ${IR_FILE};
cp ${IR_FILE} ${IR_FILE_PROF} ;
