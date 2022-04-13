all: optimized baseline

optimized: main.cpp
	clang++ -O0 $< -DOPTIMIZED=true -o $@_O0
	clang++ -O1 $< -DOPTIMIZED=true -o $@_O1
	clang++ -O2 $< -DOPTIMIZED=true -o $@_O2
	clang++ -O3 $< -DOPTIMIZED=true -o $@_O3
	clang++ -O0 $< -emit-llvm -DOPTIMIZED=true -c -o $@_O0.bc
	clang++ -O1 $< -emit-llvm -DOPTIMIZED=true -c -o $@_O1.bc
	clang++ -O2 $< -emit-llvm -DOPTIMIZED=true -c -o $@_O2.bc
	clang++ -O3 $< -emit-llvm -DOPTIMIZED=true -c -o $@_O3.bc
	llvm-dis $@_O0.bc
	llvm-dis $@_O1.bc
	llvm-dis $@_O2.bc
	llvm-dis $@_O3.bc


baseline: main.cpp
	clang++ -O0 $< -DOPTIMIZED=false -o $@_O0
	clang++ -O1 $< -DOPTIMIZED=false -o $@_O1
	clang++ -O2 $< -DOPTIMIZED=false -o $@_O2
	clang++ -O3 $< -DOPTIMIZED=false -o $@_O3
	clang++ -O0 $< -emit-llvm -DOPTIMIZED=false -c -o $@_O0.bc
	clang++ -O1 $< -emit-llvm -DOPTIMIZED=false -c -o $@_O1.bc
	clang++ -O2 $< -emit-llvm -DOPTIMIZED=false -c -o $@_O2.bc
	clang++ -O3 $< -emit-llvm -DOPTIMIZED=false -c -o $@_O3.bc
	llvm-dis $@_O0.bc
	llvm-dis $@_O1.bc
	llvm-dis $@_O2.bc
	llvm-dis $@_O3.bc

test: optimized baseline
	./test.sh

clean:
	rm -f optimized* baseline* *.bc *.ll *.txt
