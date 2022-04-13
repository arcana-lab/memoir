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

#if OPTIMIZED
struct DataStruct {
  int *a, *b, *c;

  DataStruct() {
    // Do nothing;
  }

  ~DataStruct() {
    free(a);
    free(b);
    free(c);
  };
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

#if OPTIMIZED
  auto ptr = new DataStruct();

  ptr->a = (int *)malloc(iterations * sizeof(int));
  ptr->b = (int *)malloc(iterations * sizeof(int));
  ptr->c = (int *)malloc(iterations * sizeof(int));

  for (int i = 0; i < iterations; i++) {
    ptr->a[i] = rand();
    ptr->b[i] = rand();
    ptr->c[i] = rand();
  }

#else
  auto ptr = (struct DataStruct *)malloc(
      iterations * sizeof(struct DataStruct));

  for (int i = 0; i < iterations; i++) {
    ptr[i] = DataStruct(rand(), rand(), rand());
  }
#endif

  build_end = clock();

  // MAP a
  for (int i = 0; i < iterations; i++) {
#if OPTIMIZED
    ptr->a[i % iterations] += ptr->a[(i - 1) % iterations];
#else
    ptr[i].a += ptr[(i - 1) % iterations].a;
#endif
  }

  // MAP b
  for (int i = 0; i < iterations; i++) {
#if OPTIMIZED
    ptr->b[i % iterations] += ptr->b[(i + 1) % iterations];
#else
    ptr[i].b += ptr[(i + 1) % iterations].b;
#endif
  }

  // MAP c
  for (int i = 0; i < iterations; i++) {
#if OPTIMIZED
    ptr->c[i % iterations] =
        ptr->a[i % iterations] & ptr->b[i % iterations];
#else
    ptr[i % iterations].c =
        ptr[i % iterations].a & ptr[i % iterations].b;
#endif
  }

  // REDUCE
  int max = 0;
  for (int i = 0; i < iterations; i++) {
    int c;
#if OPTIMIZED
    c = ptr->c[i % iterations];
#else
    c = ptr[i % iterations].c;
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

#if OPTIMIZED
  delete ptr;
#else
  free(ptr);
#endif

  end = clock();

  float total_time =
      1000.0 * ((float)(end - start)) / CLOCKS_PER_SEC;
  float exec_time = 1000.0 * ((float)(exec_end - build_end))
                    / CLOCKS_PER_SEC;

#if OPTIMIZED
  printf("Struct of Arrays\n");
#else
  printf("Baseline\n");
#endif
  printf("Iterations=%u\n", iterations);

  printf("TOTAL Time= %f ms\n", total_time);
  printf(" EXEC Time= %f ms\n\n", exec_time);
}
