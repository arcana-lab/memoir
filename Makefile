NOELLE_DIR=compiler/noelle
BUILD_DIR=.build.dir
INSTALL_DIR=install

all: noelle
	mkdir -p $(BUILD_DIR)
	mkdir -p $(INSTALL_DIR)
	cmake -DCMAKE_C_COMPILER=`which clang` -DCMAKE_CXX_COMPILER=`which clang++` -S . -B $(BUILD_DIR)
	make -C $(BUILD_DIR) install -j8

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
	rm -rf $(BUILD_DIR)

.PHONY: all noelle uninstall clean
