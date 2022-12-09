#!/bin/bash -e

if [ $# -lt 1 ] ; then
    echo "This script will normalize the bitcode for usage in MemOIR and NOELLE passes." ;
    echo "  USAGE: `basename $0` <INPUT IR FILE> <OUTPUT IR FILE> [<OPTIONS, ...>]" ;
fi

GIT_ROOT=`git rev-parse --show-toplevel` ;
LIB_DIR=${GIT_ROOT}/install/lib ;

source ${GIT_ROOT}/enable ;

IR_FILE_IN="$1" ;
IR_FILE_OUT="$2" ;

echo "" ;
echo "Normalization pipeline (I: ${IR_FILE_IN}, O: ${IR_FILE_OUT})" ;

IR_FILE_BASENAME=$(basename -- ${IR_FILE_IN}) ;
IR_FILE_FILENAME="${IR_FILE_BASENAME%.bc}" ;

OUT_DIR=$(dirname ${IR_FILE_IN}) ;

IR_FILE=${OUT_DIR}/${IR_FILE_FILENAME}_temp.bc ;
cp ${IR_FILE_IN} ${IR_FILE} ;

IR_FILE_NOELLE_NORM=${OUT_DIR}/${IR_FILE_FILENAME}_noelle_norm.bc ;
echo "Normalize Bitcode (I: ${IR_FILE}, O: ${IR_FILE_NOELLE_NORM})" ;
noelle-norm ${IR_FILE} -o ${IR_FILE};
cp ${IR_FILE} ${IR_FILE_NOELLE_NORM} ;

PROF_FILE="${OUT_DIR}/toProfileBinary" ;
IR_FILE_PROF=${OUT_DIR}/${IR_FILE_FILENAME}_prof.bc ;
echo "Profile Bitcode (I: ${IR_FILE}, O: ${IR_FILE_PROF})" ;
noelle-prof-coverage ${IR_FILE} ${PROF_FILE} -lm -lstdc++ ;
./${PROF_FILE} ;
mv default.profraw ${OUT_DIR}/default.profraw
noelle-meta-prof-embed ${OUT_DIR}/default.profraw ${IR_FILE} -o ${IR_FILE};
cp ${IR_FILE} ${IR_FILE_PROF} ;

cp ${IR_FILE} ${IR_FILE_OUT} ;

echo "" ;
