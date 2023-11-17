#ifndef MEMOIR_CPP_SEQUENCE_HH
#define MEMOIR_CPP_SEQUENCE_HH
#pragma once

#include <stdexcept>

#include "memoir.h"

#include "memoir++/collection.hh"
#include "memoir++/object.hh"

namespace memoir {

// Base indexed collection.
template <typename T>
class sequence : public collection {
  // static_assert(is_specialization<remove_all_pointers_t<T>, memoir::object>,
  //               "Trying to store non memoir object in a memoir collection!");
public:
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

protected:
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
    object_sequence_element(memoir::Collection *target_object, size_type idx)
      : T(MEMOIR_FUNC(index_get_struct)(target_object, idx)),
        target_object(target_object),
        idx(idx) {
      // Do nothing.
    }

    memoir::Collection *const target_object;
    const size_type idx;
  }; // class object_sequence_element

  class collection_sequence_element : public T {
  public:
    // using inner_type = typename std::remove_pointer_t<T>;

    collection_sequence_element &operator=(
        collection_sequence_element &&other) {
      this->target_object = std::move(other.target_object);
      this->idx = std::move(other.idx);
      return *this;
    }

    collection_sequence_element &operator=(collection_sequence_element other) {
      this->target_object = std::swap(this->target_object, other.target_object);
      this->idx = std::swap(this->idx, other.idx);
      return *this;
    }

    T &operator=(T &&val) const {
      if constexpr (std::is_base_of_v<memoir::collection, T>) {
        // TODO: copy construct the incoming collection
        return val;
      } else if constexpr (std::is_pointer_v<T>) {
        using inner_type = typename std::remove_pointer_t<T>;
        if constexpr (std::is_base_of_v<memoir::collection, inner_type>) {
          MEMOIR_FUNC(index_write_collection_ref)
          (val->target_object, this->target_object, this->idx);
          return val;
        }
      }
    } // T &operator=(T &&)

    operator T() const {
      if constexpr (std::is_pointer_v<T>) {
        using inner_type = typename std::remove_pointer_t<T>;
        if constexpr (std::is_base_of_v<memoir::collection, inner_type>) {
          return T(MEMOIR_FUNC(index_read_collection_ref)(this->target_object,
                                                          this->idx));
        }
      } else if constexpr (std::is_base_of_v<memoir::collection, T>) {
        return T(
            MEMOIR_FUNC(index_get_collection)(this->target_object, this->idx));
      }
    } // operator T()

    T operator*() const {
      if constexpr (std::is_pointer_v<T>) {
        using inner_type = typename std::remove_pointer_t<T>;
        if constexpr (std::is_base_of_v<memoir::collection, inner_type>) {
          return T(MEMOIR_FUNC(index_read_collection_ref)(this->target_object,
                                                          this->idx));
        }
      } else if constexpr (std::is_base_of_v<memoir::collection, T>) {
        return T(
            MEMOIR_FUNC(index_get_collection)(this->target_object, this->idx));
      } else {
        throw std::runtime_error(
            "element is not a collection or a collection reference");
      }
    } // operator*()

    collection_sequence_element(memoir::Collection *target_object,
                                size_type idx)
      : T(MEMOIR_FUNC(index_get_collection)(target_object, idx)),
        target_object(target_object),
        idx(idx) {
      // Do nothing.
    }

    memoir::Collection *const target_object;
    const size_type idx;
  }; // class collection_sequence_element

  class primitive_sequence_element {
  public:
    // using inner_type = typename std::remove_pointer_t<T>;

    // primitive_sequence_element &operator=(primitive_sequence_element &&other) {
    //   this->target_object = std::move(other.target_object);
    //   this->idx = std::move(other.idx);
    //   return *this;
    // }

    // primitive_sequence_element &operator=(primitive_sequence_element other) {
    //   this->target_object = std::swap(this->target_object, other.target_object);
    //   this->idx = std::swap(this->idx, other.idx);
    //   return *this;
    // }

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
      else {
        throw std::runtime_error("unknown sequence element type");
      }
    } // operator T()

    primitive_sequence_element(memoir::Collection *target_object, size_type idx)
      : target_object(target_object),
        idx(idx) {}

    memoir::Collection *const target_object;
    const size_type idx;
  }; // class primitive_sequence_element

public:
  using sequence_element = std::conditional_t<
      std::is_base_of_v<memoir::object, std::remove_pointer_t<T>>,
      object_sequence_element,
      std::conditional_t<
          std::is_base_of_v<memoir::collection, std::remove_pointer_t<T>>,
          collection_sequence_element,
          primitive_sequence_element>>;

  class iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type = difference_type;
    using value_type = sequence_element;
    using pointer = value_type;
    using reference = value_type;

    // Constructors.
    iterator(memoir::Collection *const storage, difference_type index)
      : _storage(storage),
        _index(index) {}

    iterator(iterator &elem) : _storage(elem._storage), _index(elem._index) {}

    // Splat.
    reference operator*() const {
      return sequence_element(this->_storage, this->_index);
    }

    pointer operator->() const {
      return sequence_element(this->_storage, this->_index);
    }

    // Prefix increment.
    iterator &operator++() {
      ++this->_index;
      return *this;
    }

    // Postfix increment.
    iterator operator++(int) {
      iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    friend bool operator==(const iterator &a, const iterator &b) {
      return (a._storage == b._storage) && (a._index == b._index);
    }

    friend bool operator!=(const iterator &a, const iterator &b) {
      return (a._storage != b._storage) || (a._index != b._index);
    }

  protected:
    memoir::Collection *const _storage;
    difference_type _index;

    friend class sequence<T>;
  }; // class iterator
  using const_iterator = const iterator;

  class reverse_iterator : public iterator {
  public:
    reverse_iterator(memoir::Collection *const storage, size_type index)
      : iterator(storage, index) {}
    reverse_iterator(reverse_iterator &iter) : iterator(iter) {}

    reverse_iterator &operator--() {
      this->_index--;
      return *this;
    }

    reverse_iterator operator--(int) {
      reverse_iterator tmp = *this;
      --(*this);
      return tmp;
    }
  };
  using const_reverse_iterator = const reverse_iterator;

  sequence(size_type n)
    : sequence(memoir::MEMOIR_FUNC(allocate_sequence)(to_memoir_type<T>(), n)) {
    // Do nothing.
  }

  sequence(memoir::Collection *seq) : collection(seq) {
    // Do nothing.
  }

  // Copy-constructor.
  sequence(const sequence &x) : sequence(x.size()) {
    for (size_type i = 0; i < x.size(); ++i) {
      (*this)[i] = (value_type)x[i];
    }
  }

  // Move-constructor.
  sequence(sequence &&x) : collection(x._storage) {}

  // TODO
  void assign(size_type count, const T &value) {
    if (this->size() != count) {
      MEMOIR_FUNC(delete_collection)(this->_storage);
      this->_storage =
          MEMOIR_FUNC(allocate_sequence)(to_memoir_type<T>(), count);
    }
    for (std::size_t i = 0; i < count; ++i) {
      this[i] = value;
    }
  }

  // Element access.
  sequence_element operator[](size_type idx) {
    return sequence_element(this->_storage, idx);
  }

  sequence_element operator[](size_type idx) const {
    return sequence_element(this->_storage, idx);
  }

  sequence_element at(size_type idx) const {
    if (idx < 0 || idx > this->size()) {
      throw std::out_of_range("sequence.at() out of bounds");
    }
    return (*this)[idx];
  }

  sequence_element front() {
    return (*this)[0];
  }

  sequence_element back() {
    return (*this)[this->size() - 1];
  }

  // Iterators.
  iterator begin() {
    return iterator(this->_storage, 0);
  }

  iterator end() {
    return iterator(this->_storage, this->size());
  }

  const_iterator cbegin() const {
    return iterator(this->_storage, 0);
  }

  const_iterator cend() const {
    return iterator(this->_storage, this->size());
  }

  reverse_iterator rbegin() {
    return reverse_iterator(this->_storage, this->size() - 1);
  }

  reverse_iterator rend() {
    return reverse_iterator(this->_storage, -1);
  }

  const_reverse_iterator crbegin() const {
    return reverse_iterator(this->_storage, this->size() - 1);
  }

  const_reverse_iterator crend() const {
    return reverse_iterator(this->_storage, -1);
  }

  // Capacity.
  size_type size() const {
    return MEMOIR_FUNC(size)(this->_storage);
  }

  bool empty() const {
    return this->size() == 0;
  }

  // Modifiers.
  void clear() {
    MEMOIR_FUNC(sequence_remove)(this->_storage, 0, this->size());
  }

  void insert(T value, size_type index) {
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

  void insert(const sequence &to_insert, size_type index) {
    MEMOIR_FUNC(sequence_insert)(to_insert._storage, this->_storage, index);
  }

  iterator insert(const_iterator pos, const T &value) {
    this->insert(pos._index, value);
    return iterator(this->_storage, pos._index);
  }

  iterator insert(const_iterator pos, T &&value) {
    this->insert(pos._index, value);
    return iterator(this->_storage, pos._index);
  }

  void remove(size_type from, size_type to) {
    MEMOIR_FUNC(sequence_remove)(this->_storage, from, to);
  }

  void remove(size_type index) {
    MEMOIR_FUNC(sequence_remove)(this->_storage, index, index + 1);
  }

  iterator erase(const_iterator pos) {
    if (pos._storage != this->_storage) {
      return pos;
    }
    if (pos == this->end()) {
      return pos;
    }
    this->remove(pos._index);
    return iterator(this->_storage, pos._index);
  }

  iterator erase(const_iterator first, const_iterator last) {
    if (this->_storage != first.storage || this->_storage != last.storage) {
      return last;
    }
    if (last == this->end()) {
      this->remove(first._index, last._index);
      return this->end();
    } else {
      this->remove(first._index, last._index);
      return last;
    }
  }

  void resize(size_type count) {
    if (count == this->size()) {
      return;
    }
    this->remove(count, this->size());
  }

  void resize(size_type count, const T &value) {
    this->assign(count, value);
  }

  // TODO
  // void swap(list<T> &other);

  void append(const sequence &to_append) {
    MEMOIR_FUNC(sequence_append)(this->_storage, to_append._storage);
  }

  void swap(size_type from_begin, size_type from_end, size_type to_begin) {
    MEMOIR_FUNC(sequence_swap)
    (this->_storage, from_begin, from_end, this->_storage, to_begin);
  }

  sequence split(size_type from, size_type to) {
    return sequence(MEMOIR_FUNC(sequence_split)(this->_storage, from, to));
  }

  sequence copy(size_type from, size_type to) {
    return sequence(MEMOIR_FUNC(sequence_slice)(this->_storage, from, to));
  }

  sequence copy() {
    return this->copy(0, this->size());
  }
}; // class sequence

} // namespace memoir

#endif // MEMOIR_CPP_SEQUENCE_HH
