#!/bin/bash -e

GIT_ROOT=`git rev-parse --show-toplevel` ;

for i in `ls` ; do
  if ! test -d "$i" ; then
    continue ;
  fi
  pushd ./ ;

  cd $i ;
  ${GIT_ROOT}/compiler/scripts/run_me.sh ;

  popd ;
done
