#!/bin/bash

if test $# -lt 1 ; then
  echo "USAGE: `basename $0` DESTINATION" ;
  exit 1;
fi
if test -e $1 ; then
  exit 1;
fi

git clone /project/parallelizing_compiler/repositories/noelle $1 ;
cd $1 ; 

make ;
