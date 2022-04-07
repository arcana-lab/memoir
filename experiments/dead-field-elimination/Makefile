all: optimized baseline

optimized:
	clang++ -O0 main.cpp -DOPTIMIZE=true -o optimized

baseline:
	clang++ -O0 main.cpp -DOPTIMIZE=false -o baseline

test: optimized baseline
	./baseline 1000000 ;
	./optimized 1000000 ;
	./baseline 100000000 ;
	./optimized 100000000

clean:
	rm -f optimized baseline
