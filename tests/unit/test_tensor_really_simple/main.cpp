#include <iostream>

#include "memoir.h"

using namespace memoir;

#define SIZE_X 100
#define SIZE_Y 200
#define ITERATIONS 100

auto tensor_type = TensorType(UInt64Type(), 2, 0, 0);

// void init(Object *tensor) {
//   assertType(tensor_type, tensor);

//   std::cout << "Initializing tensor\n";
//   for (auto x = 0; x < SIZE_X; x++) {
//     for (auto y = 0; y < SIZE_Y; y++) {
//       auto r = rand();
//       writeUInt64(getTensorElement(tensor, x, y), r);
//     }
//   }

//   return;
// }

// uint64_t do_something(Object *tensor) {
//   assertType(tensor_type, tensor);

//   std::cout << "Doing something\n";
//   for (auto i = 0; i < ITERATIONS; i++) {
//     for (uint64_t x = 0; x < SIZE_X; x++) {
//       for (uint64_t y = 0; y < SIZE_Y; y++) {
//         auto element = getTensorElement(tensor, x, y);
//         auto other_element =
//             getTensorElement(tensor, (x - 1) % SIZE_X, (y - 1) % SIZE_Y);

//         auto elem0 = readUInt64(element);
//         auto elem1 = readUInt64(other_element);

//         writeUInt64(element, elem0 - elem1);
//       }
//     }
//   }

//   auto max = 0;
  // for (uint64_t x = 0; x < SIZE_X; x++) {
  //   for (uint64_t y = 0; y < SIZE_Y; y++) {
  //     auto elem = readUInt64(getTensorElement(tensor, x, y));

  //     if (elem > max) {
  //       max = elem;
  //     }
  //   }
  // }

//   return max;
// }

int main(int argc, char **argv) {

  auto holder = allocateTensor(UInt64Type(), 2, SIZE_X, SIZE_Y);

  for (auto x = 0; x < SIZE_X; x++) {
    for (auto y = 0; y < SIZE_Y; y++) {
      auto r = x+y;
      writeUInt64(getTensorElement(holder, x, y), r);
    }
  }

  // auto max = 0;
  // for (uint64_t x = 0; x < SIZE_X; x++) {
  //   for (uint64_t y = 0; y < SIZE_Y; y++) {
  //     auto elem = readUInt64(getTensorElement(holder, x, y));
  //     if (elem > max) {
  //       max = elem;
  //     }
  //   }
  // }

  // std::cout << "Result= " << max << "\n";

  return 0;
}
