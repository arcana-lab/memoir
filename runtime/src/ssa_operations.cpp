/*
 * Object representation recognizable by LLVM IR
 * This file contains the implementation of the
 * SSA use/def PHI operations.
 *
 * Author(s): Tommy McMichen
 * Created: August 3, 2023
 */

#include "internal.h"
#include "memoir.h"
#include "utils.h"

namespace memoir {

// General-purpose renaming operations.
__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(defPHI)(const collection_ref in) {
  return in;
}

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(usePHI)(const collection_ref in) {
  return in;
}

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(argPHI)(const collection_ref in) {
  return in;
}

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(retPHI)(const collection_ref in, void *function) {
  return in;
}

// General-purpose operations.
__IMMUT_ATTR
__RUNTIME_ATTR
size_t MEMOIR_FUNC(size)(const collection_ref collection) {
  return ((detail::Collection *)collection)->size();
}

__IMMUT_ATTR
__RUNTIME_ATTR
size_t MEMOIR_FUNC(end)() {
  return -1;
}

// Fold operation.
#define HANDLE_TYPE(TYPE_NAME, C_TYPE)                                         \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(fold_##TYPE_NAME)(C_TYPE initial_value,                   \
                                       const collection_ref collection,        \
                                       void *fold_function,                    \
                                       ...) {                                  \
    MEMOIR_ASSERT(                                                             \
        false,                                                                 \
        "Fold is unimplemented in the library! Please use the compiler");      \
    return initial_value;                                                      \
  }                                                                            \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(rfold_##TYPE_NAME)(C_TYPE initial_value,                  \
                                        const collection_ref collection,       \
                                        void *fold_function,                   \
                                        ...) {                                 \
    MEMOIR_ASSERT(                                                             \
        false,                                                                 \
        "Fold is unimplemented in the library! Please use the compiler");      \
    return initial_value;                                                      \
  }
#include "types.def"

// Sequence operations.
__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(sequence_copy)(const collection_ref collection,
                                          size_t i,
                                          size_t j) {
  // Split sequence, removing elements [i,j).
  MEMOIR_ACCESS_CHECK(collection);

  MEMOIR_TYPE_CHECK(collection, TypeCode::SequenceTy);

  auto *seq = (detail::Sequence *)(collection);
  auto *seq_type = static_cast<SequenceType *>(seq->get_type());

  if (i == (size_t)-1) {
    i = seq->size();
  }

  if (j == (size_t)-1) {
    j = seq->size();
  }

  MEMOIR_ASSERT((i <= j), "Reverse split is unsupported.");

  auto m = j - i;

  std::vector<uint64_t> new_container;
  new_container.resize(m);
  std::copy(seq->begin() + i, seq->begin() + j, new_container.begin());

  seq->erase(i, j);

  return (collection_ref) new detail::SequenceAlloc(seq_type,
                                                    std::move(new_container));
}

#define HANDLE_TYPE(TYPE_NAME, C_TYPE)                                         \
  __IMMUT_ATTR                                                                 \
  __ALLOC_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  collection_ref MEMOIR_FUNC(sequence_insert_##TYPE_NAME)(                     \
      C_TYPE value,                                                            \
      const collection_ref collection,                                         \
      size_t index) {                                                          \
    /* Insert an element into a sequence. */                                   \
    MEMOIR_ACCESS_CHECK(collection);                                           \
    MEMOIR_TYPE_CHECK(collection, TypeCode::SequenceTy);                       \
    auto *seq = (detail::Sequence *)(collection);                              \
    auto *seq_type = static_cast<SequenceType *>(seq->get_type());             \
    if (index == (size_t) - 1) {                                               \
      index = seq->size();                                                     \
    }                                                                          \
                                                                               \
    std::vector<uint64_t> new_container;                                       \
    new_container.resize(seq->size() + 1);                                     \
    std::copy(seq->cbegin(), seq->cbegin() + index, new_container.begin());    \
    new_container[index] = (uint64_t)value;                                    \
    std::copy(seq->cbegin() + index,                                           \
              seq->cend(),                                                     \
              new_container.begin() + index + 1);                              \
                                                                               \
    return (collection_ref) new detail::SequenceAlloc(                         \
        seq_type,                                                              \
        std::move(new_container));                                             \
  }
#include "types.def"

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(sequence_insert)(const collection_ref collection,
                                            size_t index) {
  /* Insert an element into a sequence. */
  MEMOIR_ACCESS_CHECK(collection);
  MEMOIR_TYPE_CHECK(collection, TypeCode::SequenceTy);
  auto *seq = (detail::Sequence *)(collection);
  auto *seq_type = static_cast<SequenceType *>(seq->get_type());
  if (index == (size_t)-1) {
    index = seq->size();
  }

  std::vector<uint64_t> new_container;
  new_container.resize(seq->size() + 1);
  std::copy(seq->cbegin(), seq->cbegin() + index, new_container.begin());
  std::copy(seq->cbegin() + index,
            seq->cend(),
            new_container.begin() + index + 1);

  return (collection_ref) new detail::SequenceAlloc(seq_type,
                                                    std::move(new_container));
}

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(sequence_insert_sequence)(
    const collection_ref collection_to_insert,
    const collection_ref collection,
    size_t index) {
  MEMOIR_ACCESS_CHECK(collection);
  MEMOIR_ACCESS_CHECK(collection_to_insert);
  MEMOIR_TYPE_CHECK(collection, TypeCode::SequenceTy);
  MEMOIR_TYPE_CHECK(collection_to_insert, TypeCode::SequenceTy);
  auto *seq = (detail::Sequence *)(collection);
  auto *seq_to_insert = (detail::Sequence *)(collection_to_insert);
  auto *seq_type = static_cast<SequenceType *>(seq->get_type());

  if (index == (size_t)(-1)) {
    index = seq->size();
  }

  std::vector<uint64_t> new_container;
  new_container.resize(seq->size() + seq_to_insert->size());
  std::copy(seq->cbegin(), seq->cbegin() + index, new_container.begin());
  std::copy(seq_to_insert->cbegin(),
            seq_to_insert->cend(),
            new_container.begin() + index);
  std::copy(seq->cbegin() + index,
            seq->cend(),
            new_container.begin() + index + seq_to_insert->size());

  return (collection_ref) new detail::SequenceAlloc(seq_type,
                                                    std::move(new_container));
}

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(sequence_remove)(const collection_ref collection,
                                            size_t begin,
                                            size_t end) {
  // Remove an element from a sequence.
  MEMOIR_ACCESS_CHECK(collection);

  MEMOIR_TYPE_CHECK(collection, TypeCode::SequenceTy);

  auto *seq = (detail::Sequence *)(collection);
  auto *seq_type = static_cast<SequenceType *>(seq->get_type());

  if (begin == (size_t)(-1)) {
    begin = seq->size();
  }

  if (end == (size_t)(-1)) {
    end = seq->size();
  }

  std::vector<uint64_t> new_container;
  new_container.resize(seq->size() - (end - begin));
  std::copy(seq->cbegin(), seq->cbegin() + begin, new_container.begin());
  std::copy(seq->cbegin() + end, seq->cend(), new_container.begin() + begin);

  return (collection_ref) new detail::SequenceAlloc(seq_type,
                                                    std::move(new_container));
}

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
const collection_pair MEMOIR_FUNC(sequence_swap)(
    const collection_ref collection,
    size_t i,
    size_t j,
    const collection_ref collection2,
    size_t i2) {
  // Swap two ranges.
  MEMOIR_ACCESS_CHECK(collection);

  MEMOIR_TYPE_CHECK(collection, TypeCode::SequenceTy);
  MEMOIR_TYPE_CHECK(collection2, TypeCode::SequenceTy);

  auto *seq1 = (detail::Sequence *)(collection);
  auto *seq2 = (detail::Sequence *)(collection2);
  auto *seq_type = static_cast<SequenceType *>(seq1->get_type());

  if (i == (size_t)(-1)) {
    i = seq1->size();
  }

  if (j == (size_t)(-1)) {
    j = seq1->size();
  }

  if (i2 == (size_t)(-1)) {
    i2 = seq2->size();
  }

  MEMOIR_ASSERT((i <= j), "Reverse swap is unsupported.");

  auto m = j - i;
  auto j2 = i2 + m;

  MEMOIR_ASSERT((j2 <= seq2->size()), "Buffer overflow on copy.");

  std::vector<uint64_t> new1(seq1->cbegin(), seq1->cend());
  std::vector<uint64_t> new2(seq2->cbegin(), seq2->cend());

  auto it1 = new1.begin() + i;
  auto it2 = new2.begin() + i2;
  for (auto k = 0; k < m; ++k, ++it1, ++it2) {
    std::swap(*it1, *it2);
  }

  collection_pair pair;
  pair.first =
      (collection_ref) new detail::SequenceAlloc(seq_type, std::move(new1));
  pair.second =
      (collection_ref) new detail::SequenceAlloc(seq_type, std::move(new2));
  return pair;
}

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(sequence_swap_within)(
    const collection_ref collection,
    size_t from_begin,
    size_t from_end,
    size_t to_begin) {
  // Swap two ranges within same sequence.
  MEMOIR_ACCESS_CHECK(collection);

  MEMOIR_TYPE_CHECK(collection, TypeCode::SequenceTy);

  auto *seq = (detail::Sequence *)(collection);
  auto *seq_type = static_cast<SequenceType *>(seq->get_type());

  if (from_begin == (size_t)(-1)) {
    from_begin = seq->size();
  }

  if (from_end == (size_t)(-1)) {
    from_end = seq->size();
  }

  if (to_begin == (size_t)(-1)) {
    to_begin = seq->size();
  }

  MEMOIR_ASSERT((from_begin <= from_end), "Reverse swap is unsupported.");

  auto m = from_end - from_begin;
  auto to_end = to_begin + m;

  MEMOIR_ASSERT((to_end <= seq->size()), "Buffer overflow on copy.");

  std::vector<uint64_t> new_container;
  new_container.resize(seq->size());
  std::copy(seq->cbegin(), seq->cend(), new_container.begin());

  auto it1 = new_container.begin() + from_begin;
  auto it2 = new_container.begin() + to_begin;
  for (auto i = 0; i < m; ++i, ++it1, ++it2) {
    std::swap(*it1, *it2);
  }

  return (collection_ref) new detail::SequenceAlloc(seq_type,
                                                    std::move(new_container));
}

// Assoc operations.
__IMMUT_ATTR
__RUNTIME_ATTR
bool MEMOIR_FUNC(assoc_has)(const collection_ref collection, ...) {
  MEMOIR_ACCESS_CHECK(collection);

  va_list args;

  va_start(args, collection);

  auto result = ((detail::Collection *)collection)->has_element(args);

  va_end(args);

  return result;
}

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(assoc_remove)(const collection_ref collection, ...) {
  MEMOIR_ACCESS_CHECK(collection);

  va_list args;

  va_start(args, collection);

  auto *assoc = (detail::AssocArray *)(collection);
  auto *assoc_type = static_cast<AssocArrayType *>(assoc->get_type());

  auto *new_assoc = new detail::AssocArray(assoc_type);
  new_assoc->assoc_array.insert(assoc->assoc_array.cbegin(),
                                assoc->assoc_array.cend());
  new_assoc->remove_element(args);

  va_end(args);

  return (collection_ref)new_assoc;
}

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(assoc_insert)(const collection_ref collection, ...) {
  MEMOIR_ACCESS_CHECK(collection);

  va_list args;

  va_start(args, collection);

  auto *assoc = (detail::AssocArray *)(collection);
  auto *assoc_type = static_cast<AssocArrayType *>(assoc->get_type());

  auto *new_assoc = new detail::AssocArray(assoc_type);
  new_assoc->assoc_array.insert(assoc->assoc_array.cbegin(),
                                assoc->assoc_array.cend());
  new_assoc->get_element(args);

  va_end(args);

  return (collection_ref)new_assoc;
}

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(assoc_keys)(const collection_ref collection) {
  MEMOIR_ACCESS_CHECK(collection);

  MEMOIR_TYPE_CHECK(collection, TypeCode::AssocArrayTy);

  auto *assoc = (detail::AssocArray *)(collection);

  return (collection_ref)assoc->keys();
}

} // namespace memoir
