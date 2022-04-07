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
  int a, c, d, e, b;

  DataStruct(int a, int b, int c, int d, int e) {
    this->a = a;
    this->b = b;
    this->c = c;
    this->d = d;
    this->e = e;
  }

  void transform() {
    this->a++;
    this->c++;
    this->d++;
    this->e++;
  }
};
#else
struct DataStruct {
  int a, b, c, d, e;

  DataStruct(int a, int b, int c, int d, int e) {
    this->a = a;
    this->b = b;
    this->c = c;
    this->d = d;
    this->e = e;
  }

  void transform() {
    this->a++;
    this->c++;
    this->d++;
    this->e++;
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

    structs[i] = new DataStruct(rand(), rand(), rand(), rand(), rand());
  }

  build_end = clock();

  // MAP
  for (int i = 0; i < (1000 * iterations); i++) {
    auto datum = structs[i % iterations];

    datum->transform();
  }

  // REDUCE
  int max = 0;
  for (int i = 0; i < iterations; i++) {
    auto datum = structs[i];

#if OPTIMIZED
    int a = datum->a;
    int b = datum->b;
    int c = datum->c;
    int d = datum->d;
    int e = datum->e;

    int val = a + b + c + d + e;
#else
    int a = datum->a;
    int b = datum->b;
    int c = datum->c;
    int d = datum->d;
    int e = datum->e;

    int val = a + b + c + d + e;
#endif

    if (val > max) {
      max = val;
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
