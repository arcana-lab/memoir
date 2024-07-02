#include <iostream>

#include "cmemoir/cmemoir.h"
#include "cmemoir/test.hpp"

using namespace memoir;

int main() {
  TEST(assoc) {
    auto Map = memoir_allocate_assoc_array(memoir_i32_t, memoir_i32_t);

    // Create an identity mapping

    memoir_assoc_insert(Map, 1);
    memoir_assoc_write(i32, 10, Map, 1);
    memoir_assoc_insert(Map, 2);
    memoir_assoc_write(i32, 20, Map, 2);
    memoir_assoc_insert(Map, 3);
    memoir_assoc_write(i32, 30, Map, 3);
    memoir_assoc_insert(Map, 4);
    memoir_assoc_write(i32, 40, Map, 4);
    memoir_assoc_insert(Map, 5);
    memoir_assoc_write(i32, 50, Map, 5);

    auto res = memoir_assoc_read(i32, Map, 2);

    // If correct, this will print 20.
    EXPECT(res == 20, "Expected 20!");

    memoir_delete_collection(Map);
  }

  TEST(seq) {
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

    // If correct, this will print 5.
    EXPECT(res == 5, "Expected 5!");

    memoir_delete_collection(A);
    memoir_delete_collection(B);
  }

  return 0;
}
