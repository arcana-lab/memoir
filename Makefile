NOELLE_DIR ?= compiler/noelle
BUILD_DIR=.build.dir
HOOKS_DIR=.githooks
MEMOIR_INSTALL_DIR ?= $(shell realpath install)

all: hooks postinstall

build:
	mkdir -p $(BUILD_DIR)
	mkdir -p $(MEMOIR_INSTALL_DIR)
	cmake -DCMAKE_INSTALL_PREFIX=$(MEMOIR_INSTALL_DIR) -DCMAKE_C_COMPILER=`which clang` -DCMAKE_CXX_COMPILER=`which clang++` -S . -B $(BUILD_DIR)
	make -C $(BUILD_DIR) all -j32 --no-print-directory

install: build
	make -C $(BUILD_DIR) install -j32 --no-print-directory

postinstall: install
	@cp $(BUILD_DIR)/compile_commands.json .

benchmark: all
	make -C $(BUILD_DIR) bitcodes -j32

test:
	make -C tests/unit

documentation: all
	make -C $(BUILD_DIR)/docs documentation -j32 

hooks:
	make -C $(HOOKS_DIR) all --no-print-directory

uninstall:
	-cat $(BUILD_DIR)/install_manifest.txt | xargs rm -f
	rm -f enable
	rm -rf $(BUILD_DIR)

fulluninstall: uninstall

clean:
	make -C $(BUILD_DIR) clean -j32
	make -C $(HOOKS_DIR) clean
	rm -rf $(BUILD_DIR)

.PHONY: all noelle build install postinstall uninstall fulluninstall  clean test
