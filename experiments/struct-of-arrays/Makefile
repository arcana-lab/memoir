all: optimized baseline

optimized: main.cpp
	clang++ -O0 $< -DOPTIMIZED=true -o $@_O0
	clang++ -O1 $< -DOPTIMIZED=true -o $@_O1
	clang++ -O2 $< -DOPTIMIZED=true -o $@_O2
	clang++ -O3 $< -DOPTIMIZED=true -o $@_O3
	clang++ -O0 $< -emit-llvm -DOPTIMIZED=true -c -o $@_O0.ll
	clang++ -O1 $< -emit-llvm -DOPTIMIZED=true -c -o $@_O1.ll
	clang++ -O2 $< -emit-llvm -DOPTIMIZED=true -c -o $@_O2.ll
	clang++ -O3 $< -emit-llvm -DOPTIMIZED=true -c -o $@_O3.ll

baseline: main.cpp
	clang++ -O0 $< -DOPTIMIZED=false -o $@_O0
	clang++ -O1 $< -DOPTIMIZED=false -o $@_O1
	clang++ -O2 $< -DOPTIMIZED=false -o $@_O2
	clang++ -O3 $< -DOPTIMIZED=false -o $@_O3
	clang++ -O0 $< -emit-llvm -DOPTIMIZED=false -c -o $@_O0.ll
	clang++ -O1 $< -emit-llvm -DOPTIMIZED=false -c -o $@_O1.ll
	clang++ -O2 $< -emit-llvm -DOPTIMIZED=false -c -o $@_O2.ll
	clang++ -O3 $< -emit-llvm -DOPTIMIZED=false -c -o $@_O3.ll

test: optimized baseline
	./test.sh

clean:
	rm -f optimized baseline
