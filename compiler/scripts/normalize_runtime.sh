#!/bin/bash -e

if [ $# -lt 1 ] ; then
    echo "This script will normalize the bitcode for usage in ObjectIR and NOELLE passes." ;
    echo "  USAGE: `basename $0` <IR_FILE> [<OPTIONS, ...>]" ;
fi

GIT_ROOT=`git rev-parse --show-toplevel` ;
LIB_DIR=${GIT_ROOT}/install/lib ;

source ${GIT_ROOT}/enable ;

IR_FILE="$1" ;

echo "Normalize Runtime (I: ${IR_FILE}, O: ${IR_FILE})" ;
opt -load ${LIB_DIR}/Normalization.so -NormalizationPass -only-runtime ${IR_FILE} -o ${IR_FILE};
