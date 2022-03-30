GIT_ROOT=/home/tommy/workspace/object-ir

CC=clang++
LL=llvm-link-9

CFLAGS=-I$(GIT_ROOT)/runtime/include -std=c++17
LIBS=-lm

IR_FILE=test.bc

CFILES := $(wildcard *.c)
CPPFILES := $(wildcard *.cpp)
C_BITCODES := $(patsubst %.c,%.bc,$(CFILES))
CPP_BITCODES :=  $(patsubst %.cpp,%.bc,$(CPPFILES))

all: $(IR_FILE)

$(C_BITCODES): %.bc : %.c
	$(CC) $(CFLAGS) -emit-llvm -c $< -o $@

$(CPP_BITCODES): %.bc : %.cpp
	$(CC) $(CFLAGS) -emit-llvm -c $< -o $@

$(IR_FILE): $(C_BITCODES) $(CPP_BITCODES)
	$(LL) $^ -o $@

.PHONY: clean

clean:
	rm -f *~ /*.bc
