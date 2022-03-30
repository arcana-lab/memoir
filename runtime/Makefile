INC_DIR=include
SRC_DIR=src
BUILD_DIR=build

CC=clang++
LL=llvm-link

CFLAGS=-I$(INC_DIR)
LIBS=-lm

IR_FILE=objectir.bc

SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
BITCODES := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.bc,$(SOURCES))

all: $(BUILD_DIR)/$(IR_FILE)

$(BITCODES): $(BUILD_DIR)/%.bc : $(SRC_DIR)/%.cpp
	$(CC) $(CFLAGS) -emit-llvm -c $< -o $@

$(BUILD_DIR)/$(IR_FILE): $(BITCODES)
	$(LL) $^ -o $@

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

.PHONY: clean

clean:
	rm -f *~ $(INC_DIR)/*~ $(BUILD_DIR)/*.bc
