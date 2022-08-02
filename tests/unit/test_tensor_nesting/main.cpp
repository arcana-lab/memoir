#include <iostream>

#include "memoir.h"

using namespace memoir;

#define SIZE_X 100
#define SIZE_Y 200
#define ITERATIONS 100

auto foo_type = defineStructType("Foo",
                                 4,
                                 FloatType(),
                                 FloatType(),
                                 FloatType(),
                                 UInt64Type());

auto tensor_type = TensorType(StructType("Foo"), 2, SIZE_X, SIZE_Y);

auto holder_type =
    defineStructType("Holder", 2, StructType("Foo"), tensor_type);

void init(Object *holder) {
  assertType(StructType("Holder"), holder);

  std::cout << "Initializing holder struct\n";
  auto strct = readStruct(getStructField(holder, 0));
  writeFloat(getStructField(strct, 0), 0.0);
  writeFloat(getStructField(strct, 1), 0.0);
  writeFloat(getStructField(strct, 2), 0.0);
  writeUInt64(getStructField(strct, 3), 0);

  std::cout << "Initializing holder tensor\n";
  auto tensor = readTensor(getStructField(holder, 1));
  for (auto x = 0; x < SIZE_X; x++) {
    for (auto y = 0; y < SIZE_Y; y++) {
      auto element = readStruct(getTensorElement(tensor, x, y));

      auto r0 = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
      auto r1 = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
      auto r2 = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
      auto r3 = rand() & 15;
      writeFloat(getStructField(element, 0), r0);
      writeFloat(getStructField(element, 1), r1);
      writeFloat(getStructField(element, 2), r2);
      writeUInt64(getStructField(element, 3), r3);
    }
  }

  return;
}

void do_something(Object *holder) {
  assertType(StructType("Holder"), holder);

  std::cout << "Doing something\n";
  auto strct = readStruct(getStructField(holder, 0));
  auto tensor = readTensor(getStructField(holder, 1));

  for (auto i = 0; i < ITERATIONS; i++) {
    for (auto x = 0; x < SIZE_X; x++) {
      for (auto y = 0; y < SIZE_Y; y++) {
        auto element = readStruct(getTensorElement(tensor, x, y));

        auto elem0 = readFloat(getStructField(element, 0));
        auto elem1 = readFloat(getStructField(element, 1));
        auto elem2 = readFloat(getStructField(element, 2));
        auto elem3 = readUInt64(getStructField(element, 3));

        if (elem3 == 0) {
          writeFloat(getStructField(strct, 0), elem0);
          writeFloat(getStructField(strct, 1), elem1);
          writeFloat(getStructField(strct, 2), elem2);
          writeUInt64(getStructField(strct, 3), 1);

          writeUInt64(getStructField(element, 3), 15);
          continue;
        }

        writeFloat(getStructField(element, 0), elem0 - elem1);
        writeFloat(getStructField(element, 1), elem1 - elem2);
        writeFloat(getStructField(element, 2), elem2 - elem0);
        writeUInt64(getStructField(element, 3), elem3 - 1);
      }
    }
  }

  return;
}

void dump(Object *holder) {
  assertType(StructType("Holder"), holder);

  auto strct = readStruct(getStructField(holder, 0));

  auto read0 = readFloat(getStructField(strct, 0));
  auto read1 = readFloat(getStructField(strct, 1));
  auto read2 = readFloat(getStructField(strct, 2));
  auto read3 = readUInt64(getStructField(strct, 3));

  std::cout << "Field 1 = " << read0 << "\n";
  std::cout << "Field 2 = " << read1 << "\n";
  std::cout << "Field 3 = " << read2 << "\n";
  std::cout << "Field 4 = " << read3 << "\n";

  return;
}

int main(int argc, char **argv) {

  auto holder = allocateStruct(holder_type);

  init(holder);

  do_something(holder);

  dump(holder);

  return 0;
}
