#include <iostream>

#include "cmemoir/cmemoir.h"

using namespace memoir;

#define SIZE_X 100
#define SIZE_Y 200
#define ITERATIONS 100

auto foo_type = memoir_define_struct_type("Foo",
                                          memoir_f32_t,
                                          memoir_f32_t,
                                          memoir_f32_t,
                                          memoir_u64_t);

auto tensor_type =
    memoir_static_tensor_type(memoir_struct_type("Foo"), SIZE_X, SIZE_Y);

auto holder_type =
    memoir_define_struct_type("Holder", memoir_struct_type("Foo"), tensor_type);

void init(Struct *holder) {
  memoir_assert_struct_type(memoir_struct_type("Holder"), holder);

  std::cout << "Initializing holder struct\n";
  auto strct = memoir_struct_get(struct, holder, 0);
  memoir_struct_write(f32, 0.0, strct, 0);
  memoir_struct_write(f32, 0.0, strct, 1);
  memoir_struct_write(f32, 0.0, strct, 2);
  memoir_struct_write(u64, 0, strct, 3);

  std::cout << "Initializing holder tensor\n";
  auto tensor = memoir_struct_get(collection, holder, 1);
  for (auto x = 0; x < SIZE_X; x++) {
    for (auto y = 0; y < SIZE_Y; y++) {
      auto element = memoir_index_get(struct, tensor, x, y);

      auto r0 = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
      auto r1 = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
      auto r2 = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
      auto r3 = rand() & 15;
      memoir_struct_write(f32, r0, element, 0);
      memoir_struct_write(f32, r1, element, 1);
      memoir_struct_write(f32, r2, element, 2);
      memoir_struct_write(u64, r3, element, 3);
    }
  }

  return;
}

void do_something(Struct *holder) {
  memoir_assert_struct_type(memoir_struct_type("Holder"), holder);

  std::cout << "Doing something\n";
  auto strct = memoir_struct_get(struct, holder, 0);
  auto tensor = memoir_struct_get(collection, holder, 1);

  for (uint64_t i = 0; i < ITERATIONS; i++) {
    for (uint64_t x = 0; x < SIZE_X; x++) {
      for (uint64_t y = 0; y < SIZE_Y; y++) {
        auto element = memoir_index_get(struct, tensor, x, y);

        auto elem0 = memoir_struct_read(f32, element, 0);
        auto elem1 = memoir_struct_read(f32, element, 1);
        auto elem2 = memoir_struct_read(f32, element, 2);
        auto elem3 = memoir_struct_read(u64, element, 3);

        if (elem3 == 0) {
          memoir_struct_write(f32, elem0, strct, 0);
          memoir_struct_write(f32, elem1, strct, 1);
          memoir_struct_write(f32, elem2, strct, 2);
          memoir_struct_write(u64, 1, strct, 3);

          memoir_struct_write(u64, 15, element, 3);
          continue;
        }

        memoir_struct_write(f32, elem0 - elem1, element, 0);
        memoir_struct_write(f32, elem1 - elem2, element, 1);
        memoir_struct_write(f32, elem2 - elem0, element, 2);
        memoir_struct_write(u64, elem3 - 1, element, 3);
      }
    }
  }

  return;
}

void dump(Struct *holder) {
  memoir_assert_struct_type(memoir_struct_type("Holder"), holder);

  auto strct = memoir_struct_get(struct, holder, 0);

  auto read0 = memoir_struct_read(f32, strct, 0);
  auto read1 = memoir_struct_read(f32, strct, 1);
  auto read2 = memoir_struct_read(f32, strct, 2);
  auto read3 = memoir_struct_read(u64, strct, 3);

  std::cout << "Field 1 = " << read0 << "\n";
  std::cout << "Field 2 = " << read1 << "\n";
  std::cout << "Field 3 = " << read2 << "\n";
  std::cout << "Field 4 = " << read3 << "\n";

  return;
}

int main(int argc, char **argv) {

  auto holder = memoir_allocate_struct(holder_type);

  init(holder);

  do_something(holder);

  dump(holder);

  return 0;
}
