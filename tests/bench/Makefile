# Configuration
BENCHMARKS=mcf_s deepsjeng_s llvm-opt
NUM_RUNS=10

# Output files
ifndef COMPILE_TIMES_OUT
COMPILE_TIMES_OUT=compile_times.txt
endif
ifndef NUM_COLLECTIONS_OUT
NUM_COLLECTIONS_OUT=collections.txt
endif

# Input files
IR_FILES=$(patsubst %,%/build/normalized.bc,$(BENCHMARKS))

all: compile_times num_collections

compile_times: $(IR_FILES)
	./compile_times.sh $(COMPILE_TIMES_OUT) $(NUM_RUNS) $(BENCHMARKS)

num_collections: $(IR_FILES)
	./num_collections.sh $(NUM_COLLECTIONS_OUT) $(BENCHMARKS)

$(IR_FILES): %/build/normalized.bc: %
	make -C $< setup build/normalized.bc USE_MEMOIR=y CLANG_O3=y

clean:
	rm $(COMPILE_TIMES_OUT)
	rm $(NUM_COLLECTIONS_OUT)

.PHONY: all compile_times num_collections clean $(BENCHMARKS)
