#!/bin/bash

GIT_ROOT=`git rev-parse --show-toplevel` ;

function test_benchmark {
    TEST_DIR="$1" ;

    pushd ${TEST_DIR} ;

    echo "Running test: ${TEST_DIR}" ;

    if [ ! -f Makefile ] ; then
        cp ../Makefile.template Makefile ;
    fi

    make test ;

    popd ;
}

for arg ; do
    test_benchmark "$arg" ;
done
