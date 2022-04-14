#!/bin/bash -e

GIT_ROOT=`git rev-parse --show-toplevel` ;

function compile_benchmark {
    TEST_DIR="$1" ;

    pushd ${TEST_DIR} ;

    echo "Building test: ${TEST_DIR}"

    cp ../Makefile.template ./Makefile ;

    sed -i "s|GIT_ROOT=|GIT_ROOT=${GIT_ROOT}|" ./Makefile ;
    
    make ;

    popd ;
}

source ${GIT_ROOT}/compiler/noelle/enable ;

for arg; do
    compile_benchmark "$arg" ;
done
