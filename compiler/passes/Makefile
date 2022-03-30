all: links
	cd object_lowering ; ../../scripts/compile.sh

clean: links
	cd object_lowering ; ../../scripts/clean.sh

uninstall: clean
	rm */scripts */*.txt ;
	rm -rf build ;

.PHONY: clean links uninstall
