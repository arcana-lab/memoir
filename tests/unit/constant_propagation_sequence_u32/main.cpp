#include <iostream>

#include "cmemoir.h"

using namespace memoir;

// This is an example program that comes from
// "Bridging Control-Centric and Data-Centric Optimization" by Ben-Nun et.al.

int main() {
  auto A = memoir_allocate_sequence(memoir_i32_t, 100);
  auto B = memoir_allocate_sequence(memoir_i32_t, 100);

  for (auto i = 0; i < 100; i++) {
    // A[i] = 5
    memoir_index_write(i32, 5, A, i);
    for (auto j = 0; j < 10; j++) {
      // B[j] = A[i]
      memoir_index_write(i32, memoir_index_read(i32, A, i), B, j);
    }
    for (auto j = 0; j < 10; j++) {
      // A[j] = A[i]
      memoir_index_write(i32, memoir_index_read(i32, A, i), A, j);
    }
  }

  auto res = memoir_index_read(i32, A, 0);
  memoir_delete_collection(A);
  memoir_delete_collection(B);

  // If correct, this will print 5.
  printf("%d\n", res);

  return 0;
}
