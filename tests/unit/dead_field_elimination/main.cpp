#include <iostream>

#include "cmemoir/cmemoir.h"
#include "cmemoir/test.hpp"

using namespace memoir;

int main() {

  TEST(dead_field) {
    // Second argument is dead.
    auto myObj =
        memoir_allocate_struct(memoir_define_struct_type("DeadField",
                                                         memoir_u64_t,
                                                         memoir_u64_t,
                                                         memoir_u64_t));

    memoir_struct_write(u64, 123, myObj, 0);
    memoir_struct_write(u64, 789, myObj, 2);

    auto read1 = memoir_struct_read(u64, myObj, 0);
    auto read3 = memoir_struct_read(u64, myObj, 2);

    memoir_struct_write(u64, read1 + read3, myObj, 0);
    memoir_struct_write(u64, read3 - read1, myObj, 2);

    read1 = memoir_struct_read(u64, myObj, 0);
    read3 = memoir_struct_read(u64, myObj, 2);

    EXPECT(read1 == 912, ".0 differs!");
    EXPECT(read3 == 666, ".2 differs!");
  }

  TEST(type_escaped) {

    // type escapes, should not be eliminated.
    auto escapee =
        memoir_allocate_struct(memoir_define_struct_type("Escapes",
                                                         memoir_u64_t,
                                                         memoir_u64_t,
                                                         memoir_u64_t));
    memoir_struct_write(u64, 123, escapee, 1);
    memoir_struct_write(u64, 123, escapee, 2);

    // The following line to test the escape analysis.
    fprintf(stderr, "%p", escapee);
    fprintf(stderr, "\r                               \r");
  }

  return 0;
}
