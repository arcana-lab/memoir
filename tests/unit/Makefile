ifndef TESTS
TESTS := $(filter-out dee_sequence_copy dee_sequence_insert dee_sequence_readwrite,$(patsubst %/, %, $(sort $(wildcard */))))
endif

TEST_MAKEFILES=$(patsubst %,%/Makefile,$(TESTS))

all: $(TEST_MAKEFILES) $(TESTS)

$(TESTS): %: %/Makefile
	@printf "\e[1;2mRunning %s tests\e[0m\n" $@
	@make --no-print-directory -C $@

%/Makefile:
	[ ! -f $@ ] && cp ../Makefile.template $@

clean: 
	find $(TESTS) -type d -maxdepth 0 -exec make -C {} clean \;
	find $(TESTS) -name "Makefile" -exec rm {} \;

.PHONY: all clean $(TESTS) $(CLEAN_TESTS)

