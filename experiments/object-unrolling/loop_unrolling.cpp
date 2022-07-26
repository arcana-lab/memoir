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

#ifndef REORDER
#  define REORDER false
#endif

#ifndef INDIRECT
#  define INDIRECT false
#endif

#if OPTIMIZED
struct DataStruct {
#  if REORDER
  int a1, a2, a3, a4, b1, b2, b3, b4;
#  else
  int a1, b1, a2, b2, a3, b3, a4, b4;
#  endif

  DataStruct(int a1, int b1, int a2, int b2, int a3, int b3, int a4, int b4)
    : a1(a1),
      b1(b1),
      a2(a2),
      b2(b2),
      a3(a3),
      b3(b3),
      a4(a4),
      b4(b4) {}
};
#else
struct DataStruct {
  int a, b;

  DataStruct(int a, int b) : a(a), b(b) {}
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

  // Scale the iterations to make the experiment easier to write.
  iterations *= 4;

  start = clock();

#if OPTIMIZED
  auto num_structs = iterations / 4;
  auto structs =
#  if INDIRECT
      (struct DataStruct **)malloc(num_structs * sizeof(struct DataStruct *));
#  else
      (struct DataStruct *)malloc(num_structs * sizeof(struct DataStruct));
#  endif
  for (int i = 0; i < num_structs; i++) {
    structs[i] =
#  if INDIRECT
        new
#  endif
        DataStruct(
            // struct 1
            rand(),
            rand(),
            // struct 2
            rand(),
            rand(),
            // struct 3
            rand(),
            rand(),
            // struct 4
            rand(),
            rand());
  }

#else
  auto num_structs = iterations;
  auto structs =
#  if INDIRECT
      (struct DataStruct **)malloc(num_structs * sizeof(struct DataStruct *));
#  else
      (struct DataStruct *)malloc(num_structs * sizeof(struct DataStruct));
#  endif

  for (int i = 0; i < num_structs; i++) {
    structs[i] =
#  if INDIRECT
        new
#  endif
        DataStruct(rand(), rand());
  }
#endif

  build_end = clock();

  // MAP
  for (int i = 0; i < num_structs; i++) {
    auto datum = structs[i];

#if OPTIMIZED
#  if INDIRECT
    datum->a1++;
    datum->a2++;
    datum->a3++;
    datum->a4++;
#  else
    datum.a1++;
    datum.a2++;
    datum.a3++;
    datum.a4++;
#  endif
#else
#  if INDIRECT
    datum->a++;
#  else
    datum.a++;
#  endif
#endif
  }

  // REDUCE
  int total = 0;
  for (int i = 0; i < (num_structs / 4); i++) {
    auto datum = structs[i];
    auto datum2 = structs[i + 1];
    auto datum3 = structs[i + 2];
    auto datum4 = structs[i + 3];

#if INDIRECT
    int a = datum->a;
    int b = datum->b;
    int a2 = datum2->a;
    int b2 = datum2->b;
    int a3 = datum3->a;
    int b3 = datum3->b;
    int a4 = datum4->a;
    int b4 = datum4->b;
#else
    int a = datum.a;
    int b = datum.b;
    int a2 = datum2.a;
    int b2 = datum2.b;
    int a3 = datum3.a;
    int b3 = datum3.b;
    int a4 = datum4.a;
    int b4 = datum4.b;
#endif

    int c = a / b + a2 / b2 + a3 / b3 + a4 / b4;

    total += c;
  }

  exec_end = clock();

  std::ofstream f;
  f.open("dump.txt");
  f << "total: " << total << "\n";
  f.close();

  free(structs);

  end = clock();

  float total_time = 1000.0 * ((float)(end - start)) / CLOCKS_PER_SEC;
  float exec_time = 1000.0 * ((float)(exec_end - build_end)) / CLOCKS_PER_SEC;

#if OPTIMIZED
  printf("Object Unrolling\n");
#else
  printf("Baseline\n");
#endif
  printf("Iterations=%u\n", iterations);

  printf("TOTAL Time= %f ms\n", total_time);
  printf(" EXEC Time= %f ms\n\n", exec_time);
}
