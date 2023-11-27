SHELL := /bin/bash

# Input(s):
# BITCODE_DIR = <Directory of bitcodes>

# Configuration
BENCHMARKS ?= SPEC2017/mcf_s SPEC2017/deepsjeng_s LLVM/opt
NUM_RUNS ?= 10

# Output files
COMPILE_TIMES_OUT ?= compile_times.txt
NUM_COLLECTIONS_OUT ?= collections.txt

# Input files
IR_FILES=$(patsubst %,%/build/normalized.bc,$(BENCHMARKS))

all: compile_times num_collections

compile_times: $(IR_FILES)
	./compile_times.sh $(COMPILE_TIMES_OUT) $(NUM_RUNS) $(BENCHMARKS)

num_collections: $(IR_FILES)
	./num_collections.sh $(NUM_COLLECTIONS_OUT) $(BENCHMARKS)

$(IR_FILES): %/build/normalized.bc: % $(notdir $(dir %)) $(notdir $(dir $(dir %))) # SUITE/BENCH SUITE BENCH
	@make -B -C $< setup build/normalized.bc USE_MEMOIR=y INPUT_IR_FILE=$(shell find $(BITCODE_DIR)/$< -name "*.bc" -type f | head -1) || ( printf "\e[31;1m  ERROR: \e[97mFailed to normalize IR file for $<!\e[0m\n" ; exit 1 )

clean:
	rm $(COMPILE_TIMES_OUT)
	rm $(NUM_COLLECTIONS_OUT)

.PHONY: all compile_times num_collections clean $(BENCHMARKS)
