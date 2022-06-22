#!/bin/bash

GIT_ROOT=`git rev-parse --show-toplevel` ;

function link_makefile {
    TEST_DIR="$1" ;
    MAKEFILE=${TEST_DIR}/Makefile ;
    
    echo "Creating Makefile for $(basename ${TEST_DIR})" ;

    cp ${GIT_ROOT}/tests/Makefile.template ${MAKEFILE} ;

    sed -i "s|GIT_ROOT=|GIT_ROOT=${GIT_ROOT}|" ${MAKEFILE} ;
}

for arg; do
    link_makefile "$arg" ;
done
