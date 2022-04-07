#!/bin/bash

PERF="perf stat -e L1-dcache-misses" ;

for ITERS in 100000 ;
do
		echo "Baseline O0" ;
		${PERF} ./baseline_O0 ${ITERS} ;

		echo "Baseline O1" ;
		${PERF} ./baseline_O1 ${ITERS} ;

		echo "Baseline O2" ;
		${PERF} ./baseline_O2 ${ITERS} ;

		echo "Baseline O3" ;
		${PERF} ./baseline_O3 ${ITERS} ;



		echo "Object Inlining O0" ;
		${PERF} ./optimized_O0 ${ITERS} ;

		echo "Object Inlining O1" ;
		${PERF} ./optimized_O1 ${ITERS} ;

		echo "Object Inlining O2" ;
		${PERF} ./optimized_O2 ${ITERS} ;

		echo "Object Inlining O3" ;
		${PERF} ./optimized_O3 ${ITERS} ;
done
