#!/bin/bash -e

if [ $# -lt 1 ] ; then
    echo "This script will normalize the bitcode for usage in ObjectIR and NOELLE passes." ;
    echo "  USAGE: `basename $0` <IR_FILE> [<OPTIONS, ...>]" ;
fi

GIT_ROOT=`git rev-parse --show-toplevel` ;
LIB_DIR=${GIT_ROOT}/install/lib ;

source ${GIT_ROOT}/enable ;

IN_IR_FILE="$1" ;
TMP_IR_FILE="temp.bc" ;
OUT_IR_FILE="$2" ;

cp ${IN_IR_FILE} ${TMP_IR_FILE} ;

echo "Normalize Runtime (I: ${IN_IR_FILE}, O: ${OUT_IR_FILE})" ;

CMD="llvm-extract -glob llvm.used -rfunc memoir__* ${TMP_IR_FILE} -o ${TMP_IR_FILE}" ;
echo "$CMD" ;
eval $CMD ;

CMD="opt -strip -load ${LIB_DIR}/Normalization.so -NormalizationPass -only-runtime ${TMP_IR_FILE} -o ${OUT_IR_FILE}" ;
echo "$CMD" ;
eval $CMD ;



# rm ${TMP_IR_FILE} ;
