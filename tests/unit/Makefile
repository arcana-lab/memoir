ifndef TESTS
TESTS := $(patsubst %/, %, $(sort $(wildcard */)))
endif

TEST_MAKEFILES=$(patsubst %,%/Makefile,$(TESTS))

all: $(TEST_MAKEFILES) $(TESTS)

$(TESTS): %: %/Makefile
	make -C $@

%/Makefile:
	[ ! -f $@ ] && cp ../Makefile.template $@

clean: 
	find $(TESTS) -type d -maxdepth 0 -exec make -C {} clean \;
	find $(TESTS) -name "Makefile" -exec rm {} \;

.PHONY: all clean $(TESTS) $(CLEAN_TESTS)

