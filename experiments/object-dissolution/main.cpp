#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define DEFAULT_ITERATIONS 1000000000
#define STRUCT_SIZE 100

#ifndef OPTIMIZED
#  define OPTIMIZED false
#endif

#if OPTIMIZED
// No objects
#else
struct DataStruct {
  int a, b, c;

  DataStruct(int a, int b, int c) {
    this->a = a + b;
    this->b = b - a;
    this->c = 0;
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

  build_end = clock();

  int j = 0;
  int sum = 0;

  for (int k = 0; k < iterations; k++) {
    for (int i = 0; i < iterations; i++) {
#if OPTIMIZED
      int tempA = j++;
      int tempB = j++;
      int a = tempA + tempB;
      int b = tempB - tempA;

      int c = a + b;

      sum += c;
#else
      int tempA = j++;
      int tempB = j++;
      auto datum = new DataStruct(tempA, tempB, 0);
    
      datum->c = datum->a + datum->b;

      sum += datum->c;
#endif
    }
  }

  exec_end = clock();

  std::ofstream f;
  f.open("dump.txt");
  f << "sum: " << sum << "\n";
  f.close();

  end = clock();

  float total_time =
      1000.0 * ((float)(end - start)) / CLOCKS_PER_SEC;
  float exec_time = 1000.0 * ((float)(exec_end - build_end))
                    / CLOCKS_PER_SEC;

#if OPTIMIZED
  printf("Optimized\n");
#else
  printf("Baseline\n");
#endif
  printf("Iterations=%u\n", iterations);

  printf("TOTAL Time= %f ms\n", total_time);
  printf(" EXEC Time= %f ms\n\n", exec_time);
}
