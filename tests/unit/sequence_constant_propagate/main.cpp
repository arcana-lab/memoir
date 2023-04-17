#include <iostream>

#include "cmemoir.h"

using namespace memoir;

int main(int argc, char *argv[]) {
  auto seq = memoir_allocate_sequence(memoir_u64_t, 1);
  memoir_index_write(u64, 5, seq, 0);
  auto out = memoir_index_read(u64, seq, 0);
  printf("out = %lu\n", out);
  memoir_delete_collection(seq);
  return 0;
}
