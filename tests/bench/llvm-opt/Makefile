BENCH=llvm-opt
BITCODE_DIR=/project/memoir-bitcodes
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
O3_BACK=y
# O3_LLC=y

ifndef BUILD_DIR
BUILD_DIR=build
endif
## END USER CONFIG ##

GIT_ROOT=/nfs-scratch/jtm2066/object-ir/dev

CC=clang++
LL=llvm-link
NORM=$(GIT_ROOT)/compiler/scripts/memoir-norm
OPT=$(GIT_ROOT)/compiler/scripts/memoir-load
EXTRACT=/nfs-scratch/jtm2066/llvm-extract/install/bin/llvm-excavate --keep-going

LIBS=-lm -lstdc++
CFLAGS=-I$(GIT_ROOT)/install/include -I$(GIT_ROOT)/install/include/api -std=c++17 -O3 -fno-vectorize -fno-slp-vectorize -Xclang -disable-llvm-passes -fdeclspec $(LIBS)

IR_FILE_RUNTIME=$(GIT_ROOT)/install/lib/memoir.bc
IR_FILE_BACKENDS=$(wildcard $(GIT_ROOT)/install/lib/backend/*.bc)
IR_FILE_DECL=$(GIT_ROOT)/install/lib/memoir.decl.bc
IR_FILE_BACKENDS=$(wildcard $(GIT_ROOT)/install/lib/backend/*.bc)

IR_FILE_INPUT=$(BUILD_DIR)/input.bc

IR_FILE=$(BUILD_DIR)/program.bc

IR_FILE_NORM=$(BUILD_DIR)/normalized.bc
IR_FILE_SSA=$(BUILD_DIR)/ssa.bc
IR_FILE_SSA_OPT=$(BUILD_DIR)/ssa.optimized.bc

IR_FILE_LOWER_NORM=$(BUILD_DIR)/lowering.norm.bc
IR_FILE_LOWERED=$(BUILD_DIR)/lowered.bc
IR_FILE_LOWERED_OPT=$(BUILD_DIR)/lowered.opt.bc

IR_FILE_LLVM_STRIPPED=$(BUILD_DIR)/input.stripped.bc
IR_FILE_LINKED=$(BUILD_DIR)/linked.bc

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

BACKEND_LIB_DIR=$(GIT_ROOT)/install/lib/backend
BACKEND_LIBS=$(patsubst lib%.a,-l%,$(wildcard $(BACKEND_LIB_DIR)/lib*.a))

LLVM_LIB_DIR=/nfs-scratch/jtm2066/llvm-opt/build-no-assert/lib
# LLVM_LIBS=$(patsubst lib%.a,-l%,$(wildcard $(LLVM_LIB_DIR)/libLLVM*.a))
LLVM_LIB_NAMES=AllTargetsAsmParsers AllTargetsCodeGens AllTargetsDescs AllTargetsInfos AggressiveInstCombine Analysis BitWriter CodeGen Core Coroutines IPO IRReader InstCombine Instrumentation MC ObjCARCOpts Remarks ScalarOpts Support Target TransformUtils Vectorize Passes
# LLVM_LIBS=$(patsubst %,-lLLVM%,$(LLVM_LIB_NAMES))
LLVM_LIBS=-lLLVMSupport

LLVM_FUNCTIONS=".*isDLLImportDependent.*" ".*isThreadDependent.*"
# LLVM_FUNCTIONS=_ZL28ConstHasGlobalValuePredicatePKN4llvm8ConstantEPFbPKNS_11GlobalValueEE
# LLVM_FUNCTIONS=_ZL28ConstHasGlobalValuePredicatePKN4llvm8ConstantEPFbPKNS_11GlobalValueEE _ZL31combineInstructionsOverFunctionRN4llvm8FunctionERNS_19InstCombineWorklistEPNS_9AAResultsERNS_15AssumptionCacheERNS_17TargetLibraryInfoERNS_13DominatorTreeERNS_25OptimizationRemarkEmitterEPNS_18BlockFrequencyInfoEPNS_18ProfileSummaryInfoEbPNS_8LoopInfoE
LLVM_DELETE :=$(patsubst %,--rfunc=%,$(LLVM_FUNCTIONS))

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
	mkdir -p $@

$(C_BITCODES): $(BUILD_DIR)/%.bc : %.c
	$(CC) $(CFLAGS) -emit-llvm -c $< -o $@

$(CPP_BITCODES): $(BUILD_DIR)/%.bc : %.cpp
	$(CC) $(CFLAGS) -emit-llvm -c $< -o $@

$(IR_FILE_INPUT):
	$(EXTRACT) $(BITCODE_DIR)/$(INPUT_IR_FILE) --keep-const-init --rfunc="memoir__*" --func=main $(LLVM_DELETE) -o $@

$(IR_FILE_RUNTIME):

# MEMOIR CodeGen
$(IR_FILE_NORM): $(IR_FILE_INPUT)
	$(NORM) $< $@
ifeq ($(O3_FRONT),y)
	opt -O3 $@ -o $@
endif
	$(LL) $@ $(IR_FILE_DECL) -o $@

$(IR_FILE_SSA): $(IR_FILE_NORM)
	$(OPT) --memoir-verbose=detailed --memoir-stats --mut2immut --memoir-stats $< -o $@

$(IR_FILE_SSA_OPT): $(IR_FILE_SSA)
ifeq ($(O3_MIDDLE),y)
	opt -O3 $< -o $@
else
	cp $< $@
endif

$(IR_FILE_LOWERED): $(IR_FILE)
	$(OPT) --memoir-verbose=quick --memoir-stats --ssa-destruction --memoir-stats $< -o $@

$(IR_FILE_LOWERED_OPT): $(IR_FILE_LOWERED)
ifeq ($(O3_BACK),y)
	opt -O3 $< -o $@
else
	cp $< $@
endif

$(IR_FILE_LLVM_STRIPPED):
	$(EXTRACT) $(BITCODE_DIR)/$(INPUT_IR_FILE) --delete --recursive --recurse-on-globals $(LLVM_DELETE) -o $@

$(IR_FILE_LINKED): $(IR_FILE) $(IR_FILE_LLVM_STRIPPED)
	$(EXTRACT) --recursive --recurse-on-globals $(LLVM_DELETE) --keep-const-init $< -o $<
	$(LL) $(IR_FILE_LLVM_STRIPPED) --override $< $(IR_FILE_BACKENDS) -o $@

$(OBJ_FILE): $(IR_FILE_LINKED)
ifeq ($(O3_LLC),y)
	llc -O3 -filetype=obj $< -o $@
else
	llc -filetype=obj $< -o $@
endif

$(BINARY): $(OBJ_FILE)
	$(CC) -o $@ $< -lz -lpthread -ldl -lcurses -L$(BACKEND_LIB_DIR) $(BACKEND_LIBS) -L$(LLVM_LIB_DIR) $(LLVM_LIBS)

.PHONY: all noopt setup compile test optimize baseline lower clean $(IR_FILE)

clean:
	rm -rf $(BUILD_DIR)
