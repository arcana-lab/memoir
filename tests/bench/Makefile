BENCHMARKS=mcf_s deepsjeng_s llvm-opt

NUM_COLLECTIONS_OUT=collections.csv

IR_FILES=$(patsubst %,%/build/normalized.bc,$(BENCHMARKS))
COMPILE_TIMES=$(patsubst %,compile_times.%.csv,$(BENCHMARKS))

all: compile_times num_collections

compile_times: $(COMPILE_TIMES)

num_collections: $(IR_FILES)
	./num_collections.sh $(NUM_COLLECTIONS_OUT) $(BENCHMARKS)

$(IR_FILES): %/build/normalized.bc: %
	make -C $< setup build/normalized.bc USE_MEMOIR=y CLANG_O3=y

$(COMPILE_TIMES): compile_times.%.csv: %/build/normalized.bc
	./compile_times.sh $< $@

clean:
	rm $(COMPILE_TIMES)

.PHONY: all compile_times clean $(BENCHMARKS)
