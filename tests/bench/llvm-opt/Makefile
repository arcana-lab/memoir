GIT_ROOT=../../..

ifndef BUILD_DIR
BUILD_DIR=build
endif

CC=clang++
LL=llvm-link
OPT=$(GIT_ROOT)/compiler/scripts/memoir-opt
EXTRACT=/nfs-scratch/jtm2066/llvm-extract/install/bin/llvm-excavate --keep-going

LIBS=-lm -lstdc++
CFLAGS=-I$(GIT_ROOT)/install/include -I$(GIT_ROOT)/runtime/immer -std=c++17 -O3 -fno-vectorize -fno-slp-vectorize -Xclang -disable-llvm-passes -fdeclspec $(LIBS)

MEMOIR_IR_FILE=llvm.bc
BASELINE_IR_FILE=llvm.baseline.bc

IR_FILE=$(BUILD_DIR)/program.bc
IR_FILE_PROGRAM=$(BUILD_DIR)/linked.bc

IR_FILE_RUNTIME=$(GIT_ROOT)/install/lib/memoir.bc
IR_FILE_BACKENDS=$(wildcard $(GIT_ROOT)/install/lib/backend/*.bc)
IR_FILE_DECL=$(GIT_ROOT)/install/lib/memoir.decl.bc

IR_FILE_LLVM=$(BUILD_DIR)/llvm.bc
IR_FILE_LLVM_STRIPPED=$(BUILD_DIR)/llvm.stripped.bc
IR_FILE_INPUT=$(BUILD_DIR)/input.bc
IR_FILE_NORM=$(BUILD_DIR)/normalized.bc
IR_FILE_MEMOIR=$(BUILD_DIR)/memoir.unopt.bc
IR_FILE_MEMOIR_OPT=$(BUILD_DIR)/memoir.opt.bc
IR_FILE_LOWERED=$(BUILD_DIR)/lowered.unopt.bc

IR_FILE_UNOPT=$(BUILD_DIR)/unopt.bc
IR_FILE_OPT=$(BUILD_DIR)/optimized.bc

MEMOIR_LIB_DIR=$(GIT_ROOT)/install/lib

BITCODE_DIR=/project/memoir-bitcodes

OBJ_FILE=$(patsubst %.bc,%.o,$(IR_FILE))
BINARY=$(BUILD_DIR)/$(notdir $(realpath .))

BACKEND_LIB_DIR=$(GIT_ROOT)/install/lib/backend
BACKEND_LIBS=$(patsubst lib%.a,-l%,$(wildcard $(BACKEND_LIB_DIR)/lib*.a))

CFILES := $(wildcard *.c)
CPPFILES := $(wildcard *.cpp)
C_BITCODES := $(patsubst %.c,$(BUILD_DIR)/%.bc,$(CFILES))
CPP_BITCODES :=  $(patsubst %.cpp,$(BUILD_DIR)/%.bc,$(CPPFILES))

LLVM_FUNCTIONS=".*isDLLImportDependent.*" ".*isThreadDependent.*"
# LLVM_FUNCTIONS=_ZL28ConstHasGlobalValuePredicatePKN4llvm8ConstantEPFbPKNS_11GlobalValueEE
# LLVM_FUNCTIONS=_ZL28ConstHasGlobalValuePredicatePKN4llvm8ConstantEPFbPKNS_11GlobalValueEE _ZL31combineInstructionsOverFunctionRN4llvm8FunctionERNS_19InstCombineWorklistEPNS_9AAResultsERNS_15AssumptionCacheERNS_17TargetLibraryInfoERNS_13DominatorTreeERNS_25OptimizationRemarkEmitterEPNS_18BlockFrequencyInfoEPNS_18ProfileSummaryInfoEbPNS_8LoopInfoE
LLVM_EXTRACT := $(patsubst %,--rfunc=%:$(IR_FILE_OPT) ,$(LLVM_FUNCTIONS))
LLVM_DELETE :=$(patsubst %,--rfunc=%,$(LLVM_FUNCTIONS))

all: setup memoir-fetch memoir-optimize memoir-link memoir-binary

baseline: setup baseline-fetch baseline-optimize baseline-link baseline-binary

setup: $(BUILD_DIR)

compile: $(BUILD_DIR) $(BINARY)

test: compile
	$(BINARY) 2>&1 | tee $(BUILD_DIR)/test.out

$(BUILD_DIR):
	mkdir -p $@

$(MEMOIR_IR_FILE):
	ln -s -f $(BITCODE_DIR)/llvm-opt.bc $@

$(BASELINE_IR_FILE):
	ln -s -f $(BITCODE_DIR)/llvm-opt.baseline.bc $@

# MEMOIR pipeline.
memoir-fetch: $(MEMOIR_IR_FILE)
	cp $< $(IR_FILE_LLVM)

memoir-optimize: $(IR_FILE_LOWERED)
	cp $< $(IR_FILE_UNOPT)	

memoir-link: $(IR_FILE_OPT) $(IR_FILE_LLVM_STRIPPED) $(IR_FILE_RUNTIME) $(IR_FILE_BACKENDS)
	$(LL) $(IR_FILE_LLVM_STRIPPED) --override $(IR_FILE_OPT) $(IR_FILE_RUNTIME) $(IR_FILE_BACKENDS) -o $(IR_FILE)

memoir-binary: $(OBJ_FILE)
	$(CC) -o $(BINARY) $< -lz -lpthread -ldl -lcurses -L$(MEMOIR_LIB_DIR) -lcmemoir -lMemOIR -L$(BACKEND_LIB_DIR) $(BACKEND_LIBS) -L/project/extra/llvm/9.0.0/release/lib -lLLVMSupport

# Baseline pipeline.
baseline-fetch: $(BASELINE_IR_FILE)
	cp $< $(IR_FILE_LLVM)

baseline-optimize: $(IR_FILE_INPUT)
	$(EXTRACT) --recursive --recurse-on-globals $(LLVM_DELETE) --keep-const-init $< -o $(IR_FILE_UNOPT)

baseline-link: $(IR_FILE_OPT) $(IR_FILE_LLVM_STRIPPED)
	$(LL) $(IR_FILE_LLVM_STRIPPED) --override $(IR_FILE_OPT) -o $(IR_FILE)

baseline-binary: $(OBJ_FILE)
	$(CC) -o $(BINARY) $< -lz -lpthread -ldl -lcurses

# General-purpose setup
$(IR_FILE_LLVM_STRIPPED):
	$(EXTRACT) $(IR_FILE_LLVM) --delete --recursive --recurse-on-globals $(LLVM_DELETE) -o $@

$(IR_FILE_INPUT):
	# llvm-extract $< --rfunc="memoir__*" --rfunc=_ZL28ConstHasGlobalValuePredicatePKN4llvm8ConstantEPFbPKNS_11GlobalValueEE --func=main --rfunc=".*isDLLImportDependent.*" --rfunc=".*isThreadDependent.*" --rfunc=_ZL31combineInstructionsOverFunctionRN4llvm8FunctionERNS_19InstCombineWorklistEPNS_9AAResultsERNS_15AssumptionCacheERNS_17TargetLibraryInfoERNS_13DominatorTreeERNS_25OptimizationRemarkEmitterEPNS_18BlockFrequencyInfoEPNS_18ProfileSummaryInfoEbPNS_8LoopInfoE -o $@
	$(EXTRACT) $(IR_FILE_LLVM) --keep-const-init --rfunc="memoir__*" --func=main $(LLVM_DELETE) -o $@

# MEMOIR CodeGen
$(IR_FILE_NORM): $(IR_FILE_INPUT)
	$(OPT) --norm $< -o $@
	$(LL) $@ $(IR_FILE_DECL) $(IR_FILE_BACKENDS) -o $@

$(IR_FILE_MEMOIR): $(IR_FILE_NORM)
	memoir-load --memoir-verbose=quick --memoir-stats --mut2immut $< -o $@

$(IR_FILE_MEMOIR_OPT): $(IR_FILE_MEMOIR)
	opt -O3 $< -o $@
	# cp $< $@

$(IR_FILE_LOWERED): $(IR_FILE_MEMOIR_OPT)
	memoir-load --memoir-verbose=quick --memoir-stats -ssa-destruction --memoir-stats --memoir-dce $< -o $@
	$(EXTRACT) --recursive --recurse-on-globals $(LLVM_DELETE) --keep-const-init $@ -o $@

# General-purpose CodeGen
$(IR_FILE_OPT): $(IR_FILE_UNOPT)
	cp $< $@
	# opt -O3 $< -o $@

$(OBJ_FILE): $(IR_FILE)
	llc -O3 -filetype=obj $< -o $@

.PHONY: all noopt setup compile test optimize baseline clean memoir-fetch memoir-optimize memoir-link memoir-binary baseline-fetch baseline-optimize baseline-link baseline-binary

clean:
	rm -rf $(BUILD_DIR)
