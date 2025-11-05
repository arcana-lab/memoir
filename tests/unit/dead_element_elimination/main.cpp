#include <cassert>
#include <iostream>

#include "memoir/c/cmemoir.h"

using namespace memoir;

#define N 1000000
#define M 100

void seq_read_write() {
  auto seq = memoir_allocate_sequence(memoir_u64_t, N);

  memoir_index_write(u64, 0, seq, 0);
  for (int i = 0; i < N; ++i) {
    memoir_index_write(u64, i, seq, i);
  }

  for (int i = 0; i < 10; ++i) {
    assert(memoir_index_read(u64, seq, i) == i && "element differs!");
  }
}

void seq_copy() {
  auto seq = memoir_allocate_sequence(memoir_u64_t, N);

  for (int i = 0; i < N; ++i) {
    memoir_index_write(u64, i, seq, i);
  }

  auto cpy = memoir_seq_copy(seq, 0, M);

  for (int i = 0; i < M; ++i) {
    assert(memoir_index_read(u64, cpy, i) == i && "element differs!");
  }
}

int main() {

  seq_read_write();

  seq_copy();
}
