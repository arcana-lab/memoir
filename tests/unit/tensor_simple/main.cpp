#include <iostream>

#include "ccmemoir.h"

using namespace memoir;

#define SIZE_X 100
#define SIZE_Y 200
#define ITERATIONS 100
#define EXPECTED 2

auto tensor_type = memoir_tensor_type(memoir_u64_t, 2);

void init(Collection *tensor) {
  memoir_assert_collection_type(tensor_type, tensor);

  std::cout << "Initializing tensor\n";
  for (auto x = 0; x < SIZE_X; x++) {
    for (auto y = 0; y < SIZE_Y; y++) {
      if (y == 0) {
        memoir_index_write(u64, 1, tensor, x, y);
      } else {
        memoir_index_write(u64, 0, tensor, x, y);
      }
    }
  }

  return;
}

auto do_something(Collection *tensor) {
  memoir_assert_collection_type(tensor_type, tensor);

  std::cout << "Doing something\n";
  for (auto i = 0; i < ITERATIONS; i++) {
    for (auto x = 0; x < SIZE_X; x++) {
      for (auto y = 0; y < SIZE_Y; y++) {
        auto elem = memoir_index_read(u64, tensor, x, y);
        if (elem == 0) {
          memoir_index_write(u64, 2, tensor, x, y);
        }
      }
    }
  }

  auto max = 0;
  for (auto x = 0; x < SIZE_X; x++) {
    for (auto y = 0; y < SIZE_Y; y++) {
      auto elem = memoir_index_read(u64, tensor, x, y);

      if (elem > max) {
        max = elem;
      }
    }
  }

  return max;
}

int main(int argc, char **argv) {

  auto holder = memoir_allocate_tensor(memoir_u64_t, SIZE_X, SIZE_Y);

  init(holder);

  auto result = do_something(holder);

  std::cout << "Result= " << result << "\n";
  std::cout << "Expect= " << EXPECTED << "\n";

  if (result == EXPECTED) {
    std::cout << "TEST PASSED\n";
  } else {
    std::cout << "TEST FAILED\n";
  }

  return 0;
}
