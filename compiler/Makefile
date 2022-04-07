all: noelle passes

passes:
	make -C ./passes ;

noelle:
	./scripts/buildNOELLE.sh noelle ;

clean:
	make -C ./passes clean ;
	make -C ./noelle clean ;

.PHONY: all passes clean
