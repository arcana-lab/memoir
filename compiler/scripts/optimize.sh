#!/bin/bash -e

if [ $# -lt 1 ] ; then
    echo "This script will run ObjectIR optimizations and lower to LLVM IR." ;
    echo "  USAGE: `basename $0` <IR_FILE> [<OPTIONS, ...>]" ;
fi

GIT_ROOT=`git rev-parse --show-toplevel` ;
LIB_DIR=${GIT_ROOT}/install/lib ;

source ${GIT_ROOT}/enable ;

IR_FILE="$1" ;

echo "Running ObjectIR optimization pipeline (I: ${IR_FILE}, O: ${IR_FILE})" ;

# Normalize the bitcode
${GIT_ROOT}/compiler/scripts/normalize.sh ${IR_FILE} ;

# Lower the bitcode
${GIT_ROOT}/compiler/scripts/lower.sh ${IR_FILE} ;
