#!/bin/bash

if test $# -lt 1 ; then
  echo "USAGE: `basename $0` DESTINATION" ;
  exit 1;
fi

if ! test -d $1 ; then
  git clone git@github.com:scampanoni/noelle.git $1 ;
fi

cd $1 ;

make ;
