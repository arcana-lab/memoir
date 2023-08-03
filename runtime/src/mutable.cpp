/*
 * Object representation recognizable by LLVM IR
 * This file contains the implementation of the
 * mutable collections library.
 *
 * Author(s): Tommy McMichen
 * Created: July 12, 2023
 */

#include "internal.h"
#include "memoir.h"
#include "utils.h"

namespace memoir {
extern "C" {

// Mutable sequence operations.
#define HANDLE_TYPE(TYPE_NAME, C_TYPE)                                         \
  __RUNTIME_ATTR                                                               \
  void MEMOIR_FUNC(sequence_insert_##TYPE_NAME)(C_TYPE value,                  \
                                                Collection * collection,       \
                                                size_t index) {                \
    /* Insert an element into a sequence. */                                   \
    MEMOIR_ACCESS_CHECK(collection);                                           \
    MEMOIR_TYPE_CHECK(collection, TypeCode::SequenceTy);                       \
    auto *seq = static_cast<Sequence *>(collection);                           \
    seq->_sequence.insert(seq->_sequence.begin() + index, (uint64_t)value);    \
  }
#include "types.def"

__RUNTIME_ATTR
void MEMOIR_FUNC(sequence_remove)(Collection *collection,
                                  size_t begin,
                                  size_t end) {
  // Remove an element from a sequence.
  MEMOIR_ACCESS_CHECK(collection);

  MEMOIR_TYPE_CHECK(collection, TypeCode::SequenceTy);

  auto *seq = static_cast<Sequence *>(collection);

  seq->_sequence.erase(seq->_sequence.begin() + begin,
                       seq->_sequence.begin() + end);
}

__RUNTIME_ATTR
void MEMOIR_FUNC(sequence_append)(Collection *collection,
                                  Collection *collection_to_append) {
  // Append a sequence to another.
  MEMOIR_ACCESS_CHECK(collection);

  MEMOIR_TYPE_CHECK(collection, TypeCode::SequenceTy);
  MEMOIR_TYPE_CHECK(collection_to_append, TypeCode::SequenceTy);

  auto *seq1 = static_cast<Sequence *>(collection);
  auto *seq2 = static_cast<Sequence *>(collection_to_append);

  auto size1 = seq1->_sequence.size();
  auto size2 = seq2->_sequence.size();
  seq1->_sequence.resize(size1 + size2);

  std::copy(seq2->_sequence.begin(),
            seq2->_sequence.end(),
            seq1->_sequence.begin() + size1);

  delete seq2;
}

__RUNTIME_ATTR
void MEMOIR_FUNC(sequence_swap)(Collection *collection,
                                size_t i,
                                size_t j,
                                Collection *collection2,
                                size_t i2) {
  // Swap two ranges.
  MEMOIR_ACCESS_CHECK(collection);

  MEMOIR_TYPE_CHECK(collection, TypeCode::SequenceTy);
  MEMOIR_TYPE_CHECK(collection2, TypeCode::SequenceTy);

  auto *seq1 = static_cast<Sequence *>(collection);
  auto *seq2 = static_cast<Sequence *>(collection2);

  MEMOIR_ASSERT((i <= j), "Reverse swap is unsupported.");

  auto m = j - i;
  auto j2 = i2 + m;

  MEMOIR_ASSERT((j2 <= seq2->size()), "Buffer overflow on copy.");

  auto it1 = seq1->_sequence.begin() + i;
  auto it2 = seq2->_sequence.begin() + i2;
  for (auto k = 0; k < m; k++, ++it1, ++it2) {
    std::swap(*it1, *it2);
  }
}

__ALLOC_ATTR
__RUNTIME_ATTR
Collection *MEMOIR_FUNC(sequence_split)(Collection *collection,
                                        size_t i,
                                        size_t j) {
  // Split sequence, removing elements [i,j).
  MEMOIR_ACCESS_CHECK(collection);

  MEMOIR_TYPE_CHECK(collection, TypeCode::SequenceTy);

  auto *seq = static_cast<Sequence *>(collection);
  auto *seq_type = static_cast<SequenceType *>(collection->get_type());

  MEMOIR_ASSERT((i <= j), "Reverse split is unsupported.");

  auto m = j - i;

  std::vector<uint64_t> new_container;
  new_container.resize(m);
  std::move(seq->_sequence.begin() + i,
            seq->_sequence.begin() + j,
            new_container.begin());

  seq->_sequence.erase(seq->_sequence.begin() + i, seq->_sequence.begin() + j);

  return new Sequence(seq_type, std::move(new_container));
}

} // extern "C"
} // namespace memoir
