TESTS := $(sort $(wildcard */))

all: $(TESTS)
	echo "$(TESTS)" ;
	../scripts/compile.sh $^ ;

clean:
	rm -rf */build
