BENCH=deepsjeng_s
BITCODE_DIR=/project/memoir-bitcodes/spec2017/deepsjeng_s
INPUT_IR_FILE=$(BENCH).bc

## BEGIN USER CONFIG ##

USE_MEMOIR=y
ifeq ($(USE_MEMOIR),y)
INPUT_IR_FILE:=$(patsubst %.bc,%.memoir.bc,$(INPUT_IR_FILE))
else
INPUT_IR_FILE:=$(patsubst %.bc,%.baseline.bc,$(INPUT_IR_FILE))
endif

O3_CLANG=y
ifeq ($(O3_CLANG),y)
INPUT_IR_FILE:=$(patsubst %.bc,%.clangO3.bc,$(INPUT_IR_FILE))
else
INPUT_IR_FILE:=$(patsubst %.bc,%.clangO0.bc,$(INPUT_IR_FILE))
endif

# O3_FRONT=y
# O3_MIDDLE=y
# O3_BACK=y
# O3_LLC=y

BUILD_DIR=build
## END USER CONFIG ##

GIT_ROOT=/nfs-scratch/jtm2066/object-ir/dev

CC=clang++
LL=llvm-link
NORM=$(GIT_ROOT)/compiler/scripts/memoir-norm
OPT=$(GIT_ROOT)/compiler/scripts/memoir-load

LIBS=-lm -lstdc++
CFLAGS=-I$(GIT_ROOT)/install/include -I$(GIT_ROOT)/install/include/api -std=c++17 -O3 -fno-vectorize -fno-slp-vectorize -Xclang -disable-llvm-passes -fdeclspec $(LIBS)


IR_FILE_RUNTIME=$(GIT_ROOT)/install/lib/memoir.bc
IR_FILE_DECL=$(GIT_ROOT)/install/lib/memoir.decl.bc
IR_FILE_BACKENDS=$(wildcard $(GIT_ROOT)/install/lib/backend/*.bc)

IR_FILE=$(BUILD_DIR)/program.bc
IR_FILE_PROGRAM=$(BUILD_DIR)/linked.bc
IR_FILE_INPUT=$(BUILD_DIR)/input.bc
IR_FILE_NORM=$(BUILD_DIR)/normalized.bc
IR_FILE_SSA=$(BUILD_DIR)/ssa.bc
IR_FILE_SSA_OPT=$(BUILD_DIR)/ssa.optimized.bc
IR_FILE_LOWER_NORM=$(BUILD_DIR)/lowering.norm.bc
IR_FILE_LOWERED=$(BUILD_DIR)/lowered.bc
IR_FILE_LOWERED_OPT=$(BUILD_DIR)/lowered.opt.bc

OBJ_FILE=$(patsubst %.bc,%.o,$(IR_FILE))
BINARY:=$(BUILD_DIR)/$(patsubst %.bc,%,$(INPUT_IR_FILE))
ifeq ($(O3_FRONT),y)
BINARY:=$(BINARY).O3front
endif
ifeq ($(O3_MIDDLE),y)
BINARY:=$(BINARY).O3middle
endif
ifeq ($(O3_BACK),y)
BINARY:=$(BINARY).O3back
endif
ifeq ($(O3_LLC),y)
BINARY:=$(BINARY).O3llc
endif


all: setup optimize lower compile

noopt: setup lower compile

setup: $(BUILD_DIR)

compile: $(BUILD_DIR) $(BINARY)

test: compile
	$(BINARY) 2>&1 | tee $(BUILD_DIR)/test.out

optimize: $(IR_FILE_SSA_OPT) $(IR_FILE_BACKENDS)
	$(LL) $^ -o $(IR_FILE)

baseline: $(IR_FILE_SSA) $(IR_FILE_BACKENDS)
	$(LL) $^ -o $(IR_FILE)

lower: $(IR_FILE_LOWERED_OPT)
	opt -always-inline $< -o $(IR_FILE)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(C_BITCODES): $(BUILD_DIR)/%.bc : %.c
	$(CC) $(CFLAGS) -emit-llvm -c $< -o $@

$(CPP_BITCODES): $(BUILD_DIR)/%.bc : %.cpp
	$(CC) $(CFLAGS) -emit-llvm -c $< -o $@

$(IR_FILE_INPUT):
	cp $(BITCODE_DIR)/$(INPUT_IR_FILE) $@

$(IR_FILE_RUNTIME):

$(IR_FILE_NORM): $(IR_FILE_INPUT)
	$(NORM) $< $@
ifeq ($(O3_FRONT),y)
	opt -O3 $@ -o $@
endif
	$(LL) $@ $(IR_FILE_DECL) -o $@

$(IR_FILE_SSA): $(IR_FILE_NORM)
	$(OPT) --memoir-verbose=quick --memoir-stats --mut2immut --memoir-stats $< -o $@

$(IR_FILE_SSA_OPT): $(IR_FILE_SSA)
ifeq ($(O3_MIDDLE),y)
	opt -O3 $< -o $@
else
	cp $< $@
endif

$(IR_FILE_LOWERED): $(IR_FILE)
	$(OPT) --memoir-verbose=quick --memoir-stats --ssa-destruction --memoir-stats $< -o $@
	opt -always-inline $@ -o $@

$(IR_FILE_LOWERED_OPT): $(IR_FILE_LOWERED)
ifeq ($(O3_BACK),y)
	opt -O3 $< -o $@
else
	cp $< $@
endif

$(OBJ_FILE): $(IR_FILE)
ifeq ($(O3_LLC),y)
	llc -O3 -filetype=obj $< -o $@
else
	llc -filetype=obj $< -o $@
endif

$(BINARY): $(OBJ_FILE)
	$(CC) $< -o $@

.PHONY: all noopt setup compile test optimize baseline clean $(IR_FILE)

clean:
	rm -rf $(BUILD_DIR)
