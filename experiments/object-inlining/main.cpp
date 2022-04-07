#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define DEFAULT_ITERATIONS 1000000
#define STRUCT_SIZE 100

#ifndef OPTIMIZED
#  define OPTIMIZED false
#endif

struct ValueStruct {
  int val;

  ValueStruct(int val) {
    this->val = val;
  }
};

#if OPTIMIZED
struct DataStruct {
  int a, b;

  DataStruct(int a, int b) {
    this->a = a;
    this->b = b;
  }

  void transform() {
    this->a *= 1000;
    this->b *= 1000;
  }
};
#else
struct DataStruct {
  ValueStruct *a, *b;

  DataStruct(int a, int b) {
    this->a = new ValueStruct(a);
    this->b = new ValueStruct(b);
  }

  void transform() {
    this->a->val *= 1000;
    this->b->val *= 1000;
  }
};
#endif

void usage() {
  printf("Usage: test <num iterations>\n");
}

int main(int argc, char **argv) {

  clock_t start, build_end, exec_end, end;
  unsigned int iterations;

  if (argc == 1) {
    iterations = DEFAULT_ITERATIONS;
  } else if (argc == 2) {
    iterations = atoi(argv[1]);

    if (!iterations) {
      iterations = DEFAULT_ITERATIONS;
    }
  } else {
    usage();
  }

  start = clock();

  auto ptr = (int *)malloc(iterations * sizeof(int));
  auto structs =
      (struct DataStruct **)malloc(iterations * sizeof(struct DataStruct *));

  for (int i = 0; i < iterations; i++) {

    structs[i] = new DataStruct(rand(), rand());
  }

  build_end = clock();

  // MAP
  for (int i = 0; i < iterations; i++) {
    auto datum = structs[i];

    datum->transform();
  }

  // REDUCE
  int max = 0;
  for (int i = 0; i < iterations; i++) {
    auto datum = structs[i];

#if OPTIMIZED
    int a = datum->a;
    int b = datum->b;

    int c = a * b;
#else
    int a = datum->a->val;
    int b = datum->b->val;

    int c = a * b;
#endif

    if (c > max) {
      max = c;
    }
  }

  exec_end = clock();

  std::ofstream f;
  f.open("dump.txt");
  f << "max: " << max << "\n";
  f.close();

  for (int i = 0; i < iterations; i++) {
    delete structs[i];
  }
  free(structs);
  free(ptr);

  end = clock();

  float total_time = 1000.0 * ((float)(end - start)) / CLOCKS_PER_SEC;
  float exec_time = 1000.0 * ((float)(exec_end - build_end)) / CLOCKS_PER_SEC;

#if OPTIMIZED
  printf("Object Inlining\n");
#else
  printf("Baseline\n");
#endif
  printf("Iterations=%u\n", iterations);

  printf("TOTAL Time= %f ms\n", total_time);
  printf(" EXEC Time= %f ms\n\n", exec_time);
}
