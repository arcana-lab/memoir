ifndef TESTS
TESTS := $(patsubst %/, %, $(sort $(wildcard */)))
endif

BUILD_DIRS=$(patsubst %, %/build, $(TESTS))

all: build

links:
	../scripts/links.sh $(TESTS) ;

build: links
	../scripts/compile.sh $(TESTS) ;

test: build
	../scripts/test.sh $(TESTS) ;

.PHONY: links build test all

clean:
	rm -rf $(BUILD_DIRS)
