ifndef TESTS
TESTS := $(patsubst %/, %, $(sort $(wildcard */)))
endif

TEST_MAKEFILES=$(patsubst %,%/Makefile,$(TESTS))

all: $(TESTS)

$(TESTS): %: %/Makefile
	make -C $@

%/Makefile:
	[ ! -f $@ ] && cp ../Makefile.template $@

clean: 
	find $(TESTS) -type d -maxdepth 0 -exec make -C {} clean \;

.PHONY: all clean $(TESTS) $(CLEAN_TESTS)

