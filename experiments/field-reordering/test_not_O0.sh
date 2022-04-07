#!/bin/bash

for ITERS in 1000000  10000000 ;
do
		
		echo "Baseline O1" ;
		./baseline_O1 ${ITERS} ;

		echo "Baseline O2" ;
		./baseline_O2 ${ITERS} ;

		echo "Baseline O3" ;
		./baseline_O3 ${ITERS} ;


		
		echo "Optimized O1" ;
		./optimized_O1 ${ITERS} ;

		echo "Optimized O2" ;
		./optimized_O2 ${ITERS} ;

		echo "Optimized O3" ;
		./optimized_O3 ${ITERS} ;
done
