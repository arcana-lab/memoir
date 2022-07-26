all: run-splay

#libsplay-tree.a: splay-tree.o
#	ar ruv libsplay-tree.a splay-tree.o

#splay-tree.o: splay-tree.c splay-tree.h
#	$(CC) -Wall -O2 -c splay-tree.c

run-splay: run-splay.c splay-tree.h
	$(CC) -Wall -O2 run-splay.c splay-tree.c -o run-splay

clean:
	rm -f *.ll *.dot *.bc splay-tree.o libsplay-tree.a run-splay



