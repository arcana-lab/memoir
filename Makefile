NOELLE_DIR=compiler/noelle
BUILD_DIR=.build.dir
HOOKS_DIR=.githooks
INSTALL_DIR=install

NORM_RUNTIME=./compiler/scripts/normalize_runtime.sh
RUNTIME_BC=install/lib/memoir.bc

all: noelle hooks postinstall

build:
	mkdir -p $(BUILD_DIR)
	mkdir -p $(INSTALL_DIR)
	cmake -DCMAKE_C_COMPILER=`which clang` -DCMAKE_CXX_COMPILER=`which clang++` -S . -B $(BUILD_DIR)
	make -C $(BUILD_DIR) all -j8

install: build
	make -C $(BUILD_DIR) install -j8

postinstall: install
	# $(NORM_RUNTIME) $(RUNTIME_BC)

benchmark: all
	make -C $(BUILD_DIR) bitcodes -j8

test: all
	make -C $(BUILD_DIR) tests -j8
	ctest --tests-dir $(BUILD_DIR)

noelle: .noelle

.noelle: $(NOELLE_DIR)
	make -C $<
	touch $@

hooks:
	make -C $(HOOKS_DIR) all

$(NOELLE_DIR):
	mkdir -p $@
	git clone --depth 1 --branch v9.9.1 git@github.com:scampanoni/noelle.git $@

uninstall:
	rm -rf $(INSTALL_DIR)

clean:
	make -C $(BUILD_DIR) clean -j8
	make -C $(HOOKS_DIR) clean
	rm -rf $(BUILD_DIR)

.PHONY: all noelle build install postinstall uninstall clean test
