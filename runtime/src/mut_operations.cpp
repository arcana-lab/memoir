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
  void MUT_FUNC(sequence_insert_##TYPE_NAME)(C_TYPE value,                     \
                                             collection_ref collection,        \
                                             size_t index) {                   \
    /* Insert an element into a sequence. */                                   \
    MEMOIR_ACCESS_CHECK(collection);                                           \
    MEMOIR_TYPE_CHECK(collection, TypeCode::SequenceTy);                       \
    auto *seq = (detail::Sequence *)(collection);                              \
    seq->insert(index, (uint64_t)value);                                       \
  }
#include "types.def"

__RUNTIME_ATTR
void MUT_FUNC(sequence_insert)(collection_ref collection_to_insert,
                               collection_ref collection,
                               size_t insertion_point) {
  MEMOIR_ACCESS_CHECK(collection);
  MEMOIR_ACCESS_CHECK(collection_to_insert);
  MEMOIR_TYPE_CHECK(collection, TypeCode::SequenceTy);
  MEMOIR_TYPE_CHECK(collection_to_insert, TypeCode::SequenceTy);
  auto *seq = (detail::Sequence *)(collection);
  auto *seq_to_insert = (detail::Sequence *)(collection_to_insert);
  seq->insert(insertion_point, seq_to_insert);
}

__RUNTIME_ATTR
void MUT_FUNC(sequence_remove)(collection_ref collection,
                               size_t begin,
                               size_t end) {
  // Remove an element from a sequence.
  MEMOIR_ACCESS_CHECK(collection);

  MEMOIR_TYPE_CHECK(collection, TypeCode::SequenceTy);

  auto *seq = (detail::Sequence *)(collection);

  seq->erase(begin, end);
}

__RUNTIME_ATTR
void MUT_FUNC(sequence_append)(collection_ref collection,
                               collection_ref collection_to_append) {
  // Append a sequence to another.
  MEMOIR_ACCESS_CHECK(collection);

  MEMOIR_TYPE_CHECK(collection, TypeCode::SequenceTy);
  MEMOIR_TYPE_CHECK(collection_to_append, TypeCode::SequenceTy);

  auto *seq1 = (detail::Sequence *)(collection);
  auto *seq2 = (detail::Sequence *)(collection_to_append);

  auto size1 = seq1->size();
  auto size2 = seq2->size();
  seq1->grow(size2);

  std::copy(seq2->begin(), seq2->end(), seq1->begin() + size1);

  delete seq2;
}

__RUNTIME_ATTR
void MUT_FUNC(sequence_swap)(collection_ref collection,
                             size_t i,
                             size_t j,
                             collection_ref collection2,
                             size_t i2) {
  // Swap two ranges.
  MEMOIR_ACCESS_CHECK(collection);

  MEMOIR_TYPE_CHECK(collection, TypeCode::SequenceTy);
  MEMOIR_TYPE_CHECK(collection2, TypeCode::SequenceTy);

  auto *seq1 = (detail::Sequence *)(collection);
  auto *seq2 = (detail::Sequence *)(collection2);

  MEMOIR_ASSERT((i <= j), "Reverse swap is unsupported.");

  auto m = j - i;
  auto j2 = i2 + m;

  MEMOIR_ASSERT((j2 <= seq2->size()), "Buffer overflow on copy.");

  auto it1 = seq1->begin() + i;
  auto it2 = seq2->begin() + i2;
  for (auto k = 0; k < m; k++, ++it1, ++it2) {
    std::swap(*it1, *it2);
  }
}

__RUNTIME_ATTR
void MUT_FUNC(sequence_swap_within)(collection_ref collection,
                                    size_t i,
                                    size_t j,
                                    size_t i2) {
  // Swap two ranges.
  MEMOIR_ACCESS_CHECK(collection);

  MEMOIR_TYPE_CHECK(collection, TypeCode::SequenceTy);

  auto *seq = (detail::Sequence *)(collection);

  MEMOIR_ASSERT((i <= j), "Reverse swap is unsupported.");

  auto m = j - i;
  auto j2 = i2 + m;

  MEMOIR_ASSERT((j2 <= seq->size()), "Buffer overflow on copy.");

  auto it1 = seq->begin() + i;
  auto it2 = seq->begin() + i2;
  for (auto k = 0; k < m; k++, ++it1, ++it2) {
    std::swap(*it1, *it2);
  }
}

__RUNTIME_ATTR
collection_ref MUT_FUNC(sequence_split)(collection_ref collection,
                                        size_t i,
                                        size_t j) {
  // Split sequence, removing elements [i,j).
  MEMOIR_ACCESS_CHECK(collection);

  MEMOIR_TYPE_CHECK(collection, TypeCode::SequenceTy);

  auto *seq = (detail::Sequence *)(collection);
  auto *seq_type = static_cast<SequenceType *>(seq->get_type());

  MEMOIR_ASSERT((i <= j), "Reverse split is unsupported.");

  auto m = j - i;

  std::vector<uint64_t> new_container;
  new_container.resize(m);
  std::move(seq->begin() + i, seq->begin() + j, new_container.begin());

  seq->erase(i, j);

  return (collection_ref) new detail::SequenceAlloc(seq_type,
                                                    std::move(new_container));
}

// Assoc operations.
__RUNTIME_ATTR
void MUT_FUNC(assoc_remove)(collection_ref collection, ...) {
  MEMOIR_ACCESS_CHECK(collection);

  va_list args;

  va_start(args, collection);

  ((detail::Collection *)collection)->remove_element(args);

  va_end(args);
}

__RUNTIME_ATTR
void MUT_FUNC(assoc_insert)(collection_ref collection, ...) {
  MEMOIR_ACCESS_CHECK(collection);

  va_list args;

  va_start(args, collection);

  ((detail::Collection *)collection)->get_element(args);

  va_end(args);
}

} // extern "C"
} // namespace memoir
