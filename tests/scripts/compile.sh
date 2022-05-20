#!/bin/bash -e

GIT_ROOT=`git rev-parse --show-toplevel` ;

function compile_benchmark {
    TEST_DIR="$1" ;

    pushd ${TEST_DIR} ;

    echo "" ;
    echo "Building test: ${TEST_DIR}"
    
    make compile ;

    echo "Done building test: ${TEST_DIR}"
    echo "" ;

    popd ;
}

source ${GIT_ROOT}/compiler/noelle/enable ;

for arg; do
    compile_benchmark "$arg" ;
done
