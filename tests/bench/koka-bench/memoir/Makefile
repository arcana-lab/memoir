GIT_ROOT=$(shell git rev-parse --show-toplevel)

ifndef TESTS
TESTS := $(patsubst %/, %, $(sort $(wildcard */)))
endif

BUILD_DIRS=$(patsubst %, %/build, $(TESTS))

all: build

links:
	$(GIT_ROOT)/tests/scripts/links.sh $(TESTS) ;

build: links
	$(GIT_ROOT)/tests/scripts/compile.sh $(TESTS) ;

test: build
	$(GIT_ROOT)/tests/scripts/test.sh $(TESTS) ;

.PHONY: links build test all

clean:
	rm -rf $(BUILD_DIRS)
