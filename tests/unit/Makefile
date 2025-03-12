ifndef TESTS
# TESTS := $(filter-out c++,$(patsubst %/, %, $(sort $(wildcard */))))
# TESTS := $(patsubst %/, %, $(sort $(wildcard */)))
TESTS := assoc sequence tuple
endif

TEST_MAKEFILES=$(patsubst %,%/Makefile,$(TESTS))

all: $(TEST_MAKEFILES) clean $(TESTS)

$(TESTS): %: %/Makefile
	@printf "\e[1;2mRunning %s tests\e[0m\n" $@
	@make --no-print-directory -C $@

makefiles: $(TEST_MAKEFILES)
.PHONY: makefiles

%/Makefile:
	[ ! -f $@ ] && cp ../Makefile.template $@

clean: 
	find $(TESTS) -maxdepth 0 -type d -exec make -C {} clean \;

.PHONY: all clean $(TESTS) $(CLEAN_TESTS)

