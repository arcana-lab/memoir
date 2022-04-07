#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define DEFAULT_ITERATIONS 1000000
#define STRUCT_SIZE 100

#ifndef OBJ_INLINING
#  define OBJ_INLINING false
#endif

#if OPTIMIZE
struct DataStruct {
  int a, c;
  DataStruct(int a, int c) {
    this->a = a;
    this->c = c;
  }
};
#else
struct DataStruct {
  int a, b, c;
  DataStruct(int a, int b, int c) {
    this->a = a;
    this->b = b;
    this->c = c;
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
#if OPTIMIZE

    auto data = new DataStruct(rand(), rand());

    structs[i] = data;

#else

    auto data = new DataStruct(rand(), rand(), rand());

    structs[i] = data;

#endif
  }

  build_end = clock();

  int max = 0;
  for (int i = 0; i < iterations; i++) {

    int foo;

    auto data = structs[i];

#if OPTIMIZE

    int a = data->a;
    int c = data->c;

    foo = a * c;

#else

    int a = data->a;
    int c = data->c;

    foo = a * c;

#endif

    if (foo > max) {
      max = foo;
    }
  }

  exec_end = clock();

  std::ofstream f;
  f.open("dump.txt");
  f << max;
  f.close();

  for (int i = 0; i < iterations; i++) {
    delete structs[i];
  }
  free(structs);
  free(ptr);

  end = clock();

  float total_time = 1000.0 * ((float)(end - start)) / CLOCKS_PER_SEC;
  float exec_time = 1000.0 * ((float)(exec_end - build_end)) / CLOCKS_PER_SEC;

#if OBJ_INLINING
  printf("Object Inlining\n");
#else
  printf("Baseline\n");
#endif
  printf("Iterations=%u\n", iterations);

  printf("TOTAL Time= %f ms\n", total_time);
  printf(" EXEC Time= %f ms\n\n", exec_time);
}
