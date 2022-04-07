.PHONY: all passes noelle clean

all: passes noelle

passes:
	make -C ./passes ;

noelle:
	./scripts/buildNOELLE.sh noelle ;

clean:
	make -C ./passes clean ;
	make -C ./noelle clean ;
