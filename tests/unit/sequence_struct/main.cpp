#include <iostream>

#include "cmemoir.h"

using namespace memoir;

#define VAL0_0 1
#define VAL0_1 10
#define VAL1_0 2
#define VAL1_1 20
#define VAL2_0 3
#define VAL2_1 30

auto type = memoir_define_struct_type("Foo", memoir_u64_t, memoir_u64_t);

int main() {

  std::cout << "\nInitializing sequence\n";

  auto seq = memoir_allocate_sequence(type, 3);

  std::cout << "\nIntializing elements\n";

  auto obj0 = memoir_index_get(struct, seq, 0);
  memoir_struct_write(u64, VAL0_0, obj0, 0);
  memoir_struct_write(u64, VAL0_1, obj0, 1);
  auto obj1 = memoir_index_get(struct, seq, 1);
  memoir_struct_write(u64, VAL1_0, obj1, 0);
  memoir_struct_write(u64, VAL1_1, obj1, 1);
  auto obj2 = memoir_index_get(struct, seq, 2);
  memoir_struct_write(u64, VAL2_0, obj2, 0);
  memoir_struct_write(u64, VAL2_1, obj2, 1);

  std::cout << "\nReading sequence\n";

  obj0 = memoir_index_get(struct, seq, 0);
  auto read0_0 = memoir_struct_read(u64, obj0, 0);
  auto read0_1 = memoir_struct_read(u64, obj0, 1);
  obj1 = memoir_index_get(struct, seq, 1);
  auto read1_0 = memoir_struct_read(u64, obj1, 0);
  auto read1_1 = memoir_struct_read(u64, obj1, 1);
  obj2 = memoir_index_get(struct, seq, 2);
  auto read2_0 = memoir_struct_read(u64, obj2, 0);
  auto read2_1 = memoir_struct_read(u64, obj2, 1);

  std::cout << " Result:\n";
  std::cout << "  HEAD -> " << std::to_string(read0_0) << ", "
            << std::to_string(read0_1) << "\n";
  std::cout << "       -> " << std::to_string(read1_0) << ", "
            << std::to_string(read1_1) << "\n";
  std::cout << "       -> " << std::to_string(read2_0) << ", "
            << std::to_string(read2_1) << "\n\n";

  std::cout << "Expected:\n";
  std::cout << "  HEAD -> " << std::to_string(VAL0_0) << ", "
            << std::to_string(VAL0_1) << "\n";
  std::cout << "       -> " << std::to_string(VAL1_0) << ", "
            << std::to_string(VAL1_1) << "\n";
  std::cout << "       -> " << std::to_string(VAL2_0) << ", "
            << std::to_string(VAL2_1) << "\n\n";
}
