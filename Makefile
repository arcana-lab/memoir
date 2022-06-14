NOELLE_DIR=compiler/noelle
BUILD_DIR=.build.dir
INSTALL_DIR=install

NORM_RUNTIME=./compiler/scripts/normalize_runtime.sh
RUNTIME_BC=install/lib/object_ir.bc

all: noelle install

build:
	mkdir -p $(BUILD_DIR)
	mkdir -p $(INSTALL_DIR)
	cmake -DCMAKE_C_COMPILER=`which clang` -DCMAKE_CXX_COMPILER=`which clang++` -S . -B $(BUILD_DIR)
	make -C $(BUILD_DIR) all -j8

install: build
	make -C $(BUILD_DIR) install -j8

postinstall: install
	$(NORM_RUNTIME) $(RUNTIME_BC)

benchmark: all
	make -C $(BUILD_DIR) bitcodes -j8

test: all
	make -C $(BUILD_DIR) tests -j8
	ctest --tests-dir $(BUILD_DIR)

noelle: .noelle

.noelle: $(NOELLE_DIR)
	make -C $<
	touch $@

$(NOELLE_DIR):
	mkdir -p $@
	git clone git@github.com:scampanoni/noelle.git $@

uninstall:
	rm -rf $(INSTALL_DIR)

clean:
	make -C $(BUILD_DIR) clean -j8
	rm -rf $(BUILD_DIR)

.PHONY: all noelle build install postinstall uninstall clean test
