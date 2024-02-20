#include <iostream>

#include "cmemoir/cmemoir.h"

using namespace memoir;

int main() {
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
  printf("%d\n", res);

  memoir_delete_collection(Map);

  return 0;
}
