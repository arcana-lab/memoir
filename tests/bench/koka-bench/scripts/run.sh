#!/bin/bash

CMD=${1} ;

OUT=runs.out ;

echo "" > runs.out ;

RUNS="1 2 3 4 5 6 7 8 9 10" ;
for i in ${RUNS}
do
    echo "Run ${i}" ;
    perf stat ${CMD} 26 ;
    echo "" ;
done
