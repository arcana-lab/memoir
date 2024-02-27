NOELLE_DIR ?= compiler/noelle
BUILD_DIR=.build.dir
HOOKS_DIR=.githooks
MEMOIR_INSTALL_DIR ?= $(shell realpath install)

NORM_RUNTIME=$(MEMOIR_INSTALL_DIR)/bin/memoir-norm-runtime
RUNTIME_BC=$(MEMOIR_INSTALL_DIR)/lib/memoir.impl.bc
DECL_BC=$(MEMOIR_INSTALL_DIR)/lib/memoir.decl.bc

all: noelle hooks postinstall

build:
	mkdir -p $(BUILD_DIR)
	mkdir -p $(MEMOIR_INSTALL_DIR)
	cmake -DCMAKE_C_COMPILER=`which clang` -DCMAKE_CXX_COMPILER=`which clang++` -DCMAKE_INSTALL_PREFIX=$(MEMOIR_INSTALL_DIR) -S . -B $(BUILD_DIR)
	make -C $(BUILD_DIR) all -j32

install: build
	make -C $(BUILD_DIR) install -j32

postinstall: install
	$(NORM_RUNTIME) $(RUNTIME_BC) $(DECL_BC)
	@cp $(BUILD_DIR)/compile_commands.json .

benchmark: all
	make -C $(BUILD_DIR) bitcodes -j32

test:
	make -C $(BUILD_DIR) tests -j32
	cd $(BUILD_DIR) && ctest

noelle: .noelle

.noelle: $(NOELLE_DIR)
	NOELLE_INSTALL_DIR=$(MEMOIR_INSTALL_DIR) NOELLE_SCAF=OFF NOELLE_SVF=OFF NOELLE_AUTOTUNER=OFF make -C $<
	touch $@

hooks:
	make -C $(HOOKS_DIR) all

$(NOELLE_DIR):
	mkdir -p $@
	git clone --depth 1 --branch master git@github.com:arcana-lab/noelle.git $@

uninstall:
	-cat $(BUILD_DIR)/install_manifest.txt | xargs rm -f
	rm -f enable
	rm -rf $(BUILD_DIR)

fulluninstall: uninstall
	[[ -d compiler/noelle ]] && make -C compiler/noelle uninstall
	[[ -d compiler/noelle ]] && rm -rf compiler/noelle
	rm -f .noelle

clean:
	make -C $(BUILD_DIR) clean -j32
	make -C $(HOOKS_DIR) clean
	rm -rf $(BUILD_DIR)

.PHONY: all noelle build install postinstall uninstall fulluninstall  clean test
