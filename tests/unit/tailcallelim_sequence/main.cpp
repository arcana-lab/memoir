#include <iostream>

#include "cmemoir.h"

using namespace memoir;

Collection *gen(int n, int in[n]) {
  if (n == 0) {
    return memoir_allocate_sequence(memoir_i32_t, 0);
  }
  auto new_seq = memoir_allocate_sequence(memoir_i32_t, 1);
  memoir_index_write(i32, in[0], new_seq, 0);

  auto next_seq = gen(n - 1, in + 1);

  return memoir_join(new_seq, next_seq);
}

int main() {

  int in[100];
  for (auto i = 0; i < 100; i++) {
    in[i] = i;
  }

  auto seq = gen(100, in);

  // If correct, this will print 10.
  printf("%d\n", memoir_index_read(i32, seq, 10));

  return 0;
}
