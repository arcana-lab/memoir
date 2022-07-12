#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "splay-tree.h"

unsigned long num_keys;
unsigned long num_lookups;
long *keys;
long *lookups;

void print(splay_tree_key k, void *state) {
  printf("%ld\n", k->key);
}

#define mallocz(n) memset(malloc(n), 0, n)

int main(int argc, char *argv[]) {
  unsigned long i;

  if (argc < 3) {
    fprintf(stderr, "usage: run-splay numkeys numlookups < stream\n");
    exit(-1);
  }

  num_keys = atol(argv[1]);
  num_lookups = atol(argv[2]);

  keys = malloc(num_keys * sizeof(long));
  lookups = malloc(num_lookups * sizeof(long));

  for (i = 0; i < num_keys; i++) {
    if (scanf("insert %ld\n", &keys[i]) != 1) {
      fprintf(stderr, "failed to read insert %lu\n", i);
      exit(-1);
    }
  }

  for (i = 0; i < num_lookups; i++) {
    if (scanf("lookup %ld\n", &lookups[i]) != 1) {
      fprintf(stderr, "failed to read lookup %lu\n", i);
      exit(-1);
    }
  }

  printf("data loads done\n");

  splay_tree s = mallocz(sizeof(*s));

  for (i = 0; i < num_keys; i++) {
    splay_tree_node n = mallocz(sizeof(*n));

    n->key.key = keys[i];
    splay_tree_insert(s, n);
  }

  printf("%lu keys inserted\n", num_keys);

  struct splay_tree_key_s lookup_key;

  for (i = 0; i < num_keys; i++) {
    lookup_key.key = lookups[i];
    volatile splay_tree_key result = splay_tree_lookup(s, &lookup_key);
  }

  printf("%lu lookups done\n", num_lookups);

  //  splay_tree_foreach(s,print,0);
}
