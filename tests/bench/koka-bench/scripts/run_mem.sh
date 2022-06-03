#!/bin/bash

CMD=${1} ;

OUT=runs.out ;

echo "" > runs.out ;

NUM_RUNS=10 ;
for ((i=1; i <= NUM_RUNS; i++))
do
    echo "Run ${i}" ;
    perf stat -B -e cache-references,cache-misses,L1-dcache-loads,L1-dcache-stores,L1-dcache-load-misses,L1-icache-load-misses ${CMD} ;
    echo "" ;
done
