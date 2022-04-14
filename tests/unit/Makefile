TESTS := $(sort $(wildcard */))

all: build

build:
	../scripts/compile.sh $(TESTS) ;

test: build
	../scripts/test.sh $(TESTS) ;

clean:
	rm -rf */build
