#!/bin/bash

GIT_ROOT=`git rev-parse --show-toplevel` ;

for DIR in `ls` ; do

  if ! test -d ${DIR} ; then
    continue
  fi
    
  if [[ "${DIR}" == "build" ]] ; then
    continue ;
  fi
  
  # Prepare the links
  if ! test -e ${DIR}/run_me.sh ; then
    ln -s ${GIT_ROOT}/compiler/scripts/run_me.sh ${DIR}/run_me.sh; 
  fi

done
