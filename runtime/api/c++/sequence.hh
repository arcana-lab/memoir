#ifndef MEMOIR_CPP_SEQUENCE_HH
#define MEMOIR_CPP_SEQUENCE_HH
#pragma once

#include "memoir.h"

#include "object.hh"

namespace memoir {

// Base element.
class element {
  // Nothing.
};

// Base indexed collection.
template <typename T>
class sequence {
  // static_assert(is_specialization<remove_all_pointers_t<T>, memoir::object>,
  //               "Trying to store non memoir object in a memoir collection!");

  class object_sequence_element : public T {
  public:
    // using inner_type = typename std::remove_pointer_t<T>;

    object_sequence_element &operator=(object_sequence_element &&other) {
      this->target_object = std::move(other.target_object);
      this->idx = std::move(other.idx);
      return *this;
    }

    object_sequence_element &operator=(object_sequence_element other) {
      this->target_object = std::swap(this->target_object, other.target_object);
      this->idx = std::swap(this->idx, other.idx);
      return *this;
    }

    T &operator=(T &&val) const {
      if constexpr (std::is_base_of_v<memoir::object, T>) {
        // TODO: copy construct the incoming struct
        return val;
      } else if constexpr (std::is_pointer_v<T>) {
        using inner_type = typename std::remove_pointer_t<T>;
        if constexpr (std::is_base_of_v<memoir::object, inner_type>) {
          MEMOIR_FUNC(index_write_struct_ref)
          (val->target_object, this->target_object, this->idx);
          return val;
        }
      }
    } // T &operator=(T &&)

    operator T() const {
      if constexpr (std::is_pointer_v<T>) {
        using inner_type = typename std::remove_pointer_t<T>;
        if constexpr (std::is_base_of_v<memoir::object, inner_type>) {
          return T(MEMOIR_FUNC(index_read_struct_ref)(this->target_object,
                                                      this->idx));
        }
      } else if constexpr (std::is_base_of_v<memoir::object, T>) {
        return T(MEMOIR_FUNC(index_get_struct)(this->target_object, this->idx));
      }
    } // operator T()

    T operator*() const {
      if constexpr (std::is_pointer_v<T>) {
        using inner_type = typename std::remove_pointer_t<T>;
        if constexpr (std::is_base_of_v<memoir::object, inner_type>) {
          return T(MEMOIR_FUNC(index_read_struct_ref)(this->target_object,
                                                      this->idx));
        }
      } else if constexpr (std::is_base_of_v<memoir::object, T>) {
        return T(MEMOIR_FUNC(index_get_struct)(this->target_object, this->idx));
      }
    } // operator T()

    // TODO: make this construct the underlying object with a get_struct
    object_sequence_element(memoir::Collection *target_object, std::size_t idx)
      : T(MEMOIR_FUNC(index_get_struct)(target_object, idx)),
        target_object(target_object),
        idx(idx) {
      // Do nothing.
    }

    memoir::Collection *const target_object;
    const std::size_t idx;
  }; // class object_sequence_element

  class primitive_sequence_element {
  public:
    // using inner_type = typename std::remove_pointer_t<T>;

    primitive_sequence_element &operator=(primitive_sequence_element &&other) {
      this->target_object = std::move(other.target_object);
      this->idx = std::move(other.idx);
      return *this;
    }

    primitive_sequence_element &operator=(primitive_sequence_element other) {
      this->target_object = std::swap(this->target_object, other.target_object);
      this->idx = std::swap(this->idx, other.idx);
      return *this;
    }

    T operator=(T val) const {
      if constexpr (std::is_pointer_v<T>) {
        MEMOIR_FUNC(index_write_ptr)
        (val, this->target_object, this->idx);
        return val;
      }
#define HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)                            \
  else if constexpr (std::is_same_v<T, C_TYPE>) {                              \
    MEMOIR_FUNC(index_write_##TYPE_NAME)                                       \
    (val, this->target_object, this->idx);                                     \
    return val;                                                                \
  }
#define HANDLE_INTEGER_TYPE(TYPE_NAME, C_TYPE, BW, IS_SIGNED)                  \
  HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)
#include <types.def>
#undef HANDLE_PRIMITIVE_TYPE
#undef HANDLE_INTEGER_TYPE
    } // T &operator=(T &&)

    operator T() const {
      if constexpr (std::is_pointer_v<T>) {
        return MEMOIR_FUNC(index_read_ptr)(this->target_object, this->idx);
      }
#define HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)                            \
  else if constexpr (std::is_same_v<T, C_TYPE>) {                              \
    return MEMOIR_FUNC(index_read_##TYPE_NAME)(this->target_object,            \
                                               this->idx);                     \
  }
#define HANDLE_INTEGER_TYPE(TYPE_NAME, C_TYPE, BW, IS_SIGNED)                  \
  HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)
#include <types.def>
#undef HANDLE_PRIMITIVE_TYPE
#undef HANDLE_INTEGER_TYPE
    } // operator T()

    T operator*() const {
      if constexpr (std::is_pointer_v<T>) {
        return MEMOIR_FUNC(index_read_ptr)(this->target_object, this->idx);
      }
#define HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)                            \
  else if constexpr (std::is_same_v<T, C_TYPE>) {                              \
    return MEMOIR_FUNC(index_read_##TYPE_NAME)(this->target_object,            \
                                               this->idx);                     \
  }
#define HANDLE_INTEGER_TYPE(TYPE_NAME, C_TYPE, BW, IS_SIGNED)                  \
  HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)
#include <types.def>
#undef HANDLE_PRIMITIVE_TYPE
#undef HANDLE_INTEGER_TYPE
    } // operator T()

    primitive_sequence_element(memoir::Collection *target_object,
                               std::size_t idx)
      : target_object(target_object),
        idx(idx) {}

    memoir::Collection *const target_object;
    const std::size_t idx;
  }; // class primitive_sequence_element

  using sequence_element = std::conditional_t<
      std::is_base_of_v<memoir::object, std::remove_pointer_t<T>>,
      object_sequence_element,
      primitive_sequence_element>;

  class sequence_iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::size_t;
    using value_type = sequence_element;
    using pointer = value_type;
    using reference = value_type;

    // Constructors.
    sequence_iterator(memoir::Collection *const storage, std::size_t index)
      : _storage(storage),
        _index(index) {}

    sequence_iterator(sequence_iterator &elem)
      : _storage(elem._storage),
        _index(elem._index) {}

    // Splat.
    reference operator*() const {
      return sequence_element(this->_storage, this->_index);
    }

    pointer operator->() const {
      return sequence_element(this->_storage, this->_index);
    }

    // Prefix increment.
    sequence_iterator &operator++() {
      this->_index++;
      return *this;
    }

    // Postfix increment.
    sequence_iterator operator++(int) {
      sequence_iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    friend bool operator==(const sequence_iterator &a,
                           const sequence_iterator &b) {
      return (a._storage == b._storage) && (a._index == b._index);
    }

    friend bool operator!=(const sequence_iterator &a,
                           const sequence_iterator &b) {
      return (a._storage != b._storage) || (a._index != b._index);
    }

  protected:
    memoir::Collection *const _storage;
    std::size_t _index;
  }; // class sequence_iterator

  class sequence_bidi_iterator : public sequence_iterator {
  public:
    sequence_bidi_iterator(memoir::Collection *const storage, std::size_t index)
      : sequence_iterator(storage, index) {}
    sequence_bidi_iterator(sequence_bidi_iterator &iter)
      : sequence_iterator(iter) {}

    sequence_bidi_iterator &operator--() {
      this->_index--;
      return *this;
    }

    sequence_bidi_iterator operator--(int) {
      sequence_bidi_iterator tmp = *this;
      --(*this);
      return tmp;
    }
  };

public:
  sequence(std::size_t n)
    : _storage(memoir::MEMOIR_FUNC(allocate_sequence)(to_memoir_type<T>(), n)) {
    // Do nothing.
  }

  sequence(memoir::Collection *seq) : _storage(seq) {
    // Do nothing.
  }

  sequence_element operator[](std::size_t idx) {
    return sequence_element(this->_storage, idx);
  }

  sequence_element operator[](std::size_t idx) const {
    return sequence_element(this->_storage, idx);
  }

  sequence_iterator begin() {
    return sequence_iterator(this->_storage, 0);
  }

  sequence_iterator end() {
    return sequence_iterator(this->_storage, this->size());
  }

  sequence_bidi_iterator rbegin() {
    return sequence_bidi_iterator(this->_storage, this->size() - 1);
  }

  sequence_bidi_iterator rend() {
    return sequence_bidi_iterator(this->_storage, -1);
  }

  std::size_t size() const {
    return MEMOIR_FUNC(size)(this->_storage);
  }

  void insert(T value, std::size_t index) {
    if constexpr (std::is_pointer_v<T>) {
      using inner_type = typename std::remove_pointer_t<T>;
      if constexpr (is_specialization<inner_type, memoir::object>) {
        MEMOIR_FUNC(sequence_insert_struct_ref)(value, this->_storage, index);
      } else {
        MEMOIR_FUNC(sequence_insert_ptr)(value, this->_storage, index);
      }
    }
#define HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)                            \
  else if constexpr (std::is_same_v<T, C_TYPE>) {                              \
    return MEMOIR_FUNC(                                                        \
        sequence_insert_##TYPE_NAME)(value, this->_storage, index);            \
  }
#define HANDLE_INTEGER_TYPE(TYPE_NAME, C_TYPE, BW, IS_SIGNED)                  \
  HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)
#include <types.def>
#undef HANDLE_PRIMITIVE_TYPE
#undef HANDLE_INTEGER_TYPE
  }

  void insert(const sequence &to_insert, std::size_t index) {
    MEMOIR_FUNC(sequence_insert)(to_insert._storage, this->_storage, index);
  }

  void remove(std::size_t from, std::size_t to) {
    MEMOIR_FUNC(sequence_remove)(this->_storage, from, to);
  }

  void remove(std::size_t index) {
    MEMOIR_FUNC(sequence_remove)(this->_storage, index, index + 1);
  }

  void append(const sequence &to_append) {
    MEMOIR_FUNC(sequence_append)(this->_storage, to_append._storage);
  }

  void swap(std::size_t from_begin,
            std::size_t from_end,
            std::size_t to_begin) {
    MEMOIR_FUNC(sequence_swap)
    (this->_storage, from_begin, from_end, this->_storage, to_begin);
  }

  sequence split(std::size_t from, std::size_t to) {
    return sequence(MEMOIR_FUNC(sequence_split)(this->_storage, from, to));
  }

  sequence copy(std::size_t from, std::size_t to) {
    return sequence(MEMOIR_FUNC(sequence_slice)(this->_storage, from, to));
  }

  sequence copy() {
    return this->copy(0, this->size());
  }

protected:
  memoir::Collection *const _storage;
}; // class sequence

} // namespace memoir

#endif // MEMOIR_CPP_SEQUENCE_HH
