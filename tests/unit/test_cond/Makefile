GIT_ROOT=/home/tommy/workspace/object-ir

BUILD_DIR=build

CC=clang++
LL=llvm-link
OPT=$(GIT_ROOT)/compiler/scripts/optimize.sh

CFLAGS=-I$(GIT_ROOT)/runtime/include -std=c++17
LIBS=-lm

IR_FILE=$(BUILD_DIR)/all_in_one.bc
OBJ_FILE=$(patsubst %.bc,%.o,$(IR_FILE))
BINARY=$(patsubst %.bc,%,$(IR_FILE))


CFILES := $(wildcard *.c)
CPPFILES := $(wildcard *.cpp)
C_BITCODES := $(patsubst %.c,$(BUILD_DIR)/%.bc,$(CFILES))
CPP_BITCODES :=  $(patsubst %.cpp,$(BUILD_DIR)/%.bc,$(CPPFILES))

all: $(BUILD_DIR) $(BINARY)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(C_BITCODES): $(BUILD_DIR)/%.bc : %.c
	$(CC) $(CFLAGS) -emit-llvm -c $< -o $@

$(CPP_BITCODES): $(BUILD_DIR)/%.bc : %.cpp
	$(CC) $(CFLAGS) -emit-llvm -c $< -o $@

$(IR_FILE): $(C_BITCODES) $(CPP_BITCODES)
	$(LL) $^ $(GIT_ROOT)/runtime/build/objectir.bc -o $@
	$(OPT) $@

$(OBJ_FILE): $(IR_FILE)
	llc -filetype=obj $< -o $@

$(BINARY): $(OBJ_FILE)
	$(CC) $< -o $@

.PHONY: clean

clean:
	rm -f *~ *.bc
