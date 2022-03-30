#!/bin/bash -e

GIT_ROOT=`git rev-parse --show-toplevel` ;

export CC=clang
export CXX=clang++

rm -rf build/ ;
mkdir build ;
cd build ; 
cmake -DCMAKE_INSTALL_PREFIX="${GIT_ROOT}/compiler/passes/build" -DCMAKE_BUILD_TYPE=Debug ../ ; 
make ;
make install ;
cd ../ 

# cp build/compile_commands.json ./ ;
