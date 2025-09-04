#ifndef MEMOIR_CPP_SEQUENCE_HH
#define MEMOIR_CPP_SEQUENCE_HH
#pragma once

#include <algorithm>
#include <stdexcept>

#include "memoir.h"

#include "memoir++/collection.hh"
#include "memoir++/element.hh"
#include "memoir++/object.hh"

namespace memoir {

// Base indexed collection.
template <typename T>
class Seq : public collection {
  // static_assert(is_instance_of_v<remove_all_pointers_t<T>, memoir::object>,
  //               "Trying to store non memoir object in a memoir
  //               collection!");
public:
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

protected:
public:
  using sequence_element = Element<T, size_t>;

  class iterator {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = difference_type;
    using value_type = sequence_element;
    using pointer = value_type;
    using reference = value_type;

    // Constructors.
    always_inline iterator(memoir::Collection *const storage,
                           difference_type index)
      : _storage(storage),
        _index(index) {}

    always_inline iterator(const iterator &elem)
      : _storage(elem._storage),
        _index(elem._index) {}

    // Copy operator
    always_inline iterator &operator=(const iterator &other) = default;

    // Splat.
    always_inline reference operator*() const {
      return sequence_element(this->_storage, this->_index);
    }

    always_inline pointer operator->() const {
      return sequence_element(this->_storage, this->_index);
    }

    // Prefix increment.
    always_inline iterator &operator++() {
      ++this->_index;
      return *this;
    }

    // Postfix increment.
    always_inline iterator operator++(int) {
      iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    // Prefix decrement.
    always_inline iterator &operator--() {
      --this->_index;
      return *this;
    }

    // Postfix decrement.
    always_inline iterator operator--(int) {
      iterator tmp = *this;
      --(*this);
      return tmp;
    }

    // Arithmetic.
    always_inline iterator &operator+=(difference_type offset) {
      this->_index += offset;
      return *this;
    }

    always_inline friend iterator operator+(const iterator &iter,
                                            difference_type offset) {
      return iterator(iter._storage, iter._index + offset);
    }

    always_inline friend iterator operator+(difference_type offset,
                                            const iterator &iter) {
      return iter + offset;
    }

    always_inline iterator &operator-=(difference_type offset) {
      this->_index -= offset;
      return *this;
    }

    always_inline friend iterator operator-(const iterator &iter,
                                            difference_type offset) {
      return iterator(iter._storage, iter._index - offset);
    }

    always_inline friend iterator operator-(difference_type base,
                                            const iterator &iter) {
      return iterator(iter._storage, base - iter._index);
    }

    always_inline friend difference_type operator-(const iterator &a,
                                                   const iterator &b) {
      return a._index - b._index;
    }

    // always_inline operator difference_type() const {
    //   return this->_index;
    // }

    // Comparison.
    always_inline friend bool operator==(const iterator &a, const iterator &b) {
      return (a._storage == b._storage) && (a._index == b._index);
    }

    always_inline friend bool operator!=(const iterator &a, const iterator &b) {
      return (a._storage != b._storage) || (a._index != b._index);
    }

    always_inline friend bool operator<(const iterator &a, const iterator &b) {
      return a._index < b._index;
    }

    always_inline friend bool operator<(const iterator &a, difference_type b) {
      return a._index < b;
    }

    always_inline friend bool operator>(const iterator &a, const iterator &b) {
      return a._index > b._index;
    }

    always_inline friend bool operator>(const iterator &a, difference_type b) {
      return a._index > b;
    }

    always_inline friend bool operator<=(const iterator &a, const iterator &b) {
      return a._index <= b._index;
    }

    always_inline friend bool operator<=(const iterator &a, difference_type b) {
      return a._index <= b;
    }

    always_inline friend bool operator>=(const iterator &a, const iterator &b) {
      return a._index >= b._index;
    }

    always_inline friend bool operator>=(const iterator &a, difference_type b) {
      return a._index >= b;
    }

  protected:
    memoir::Collection *_storage;
    difference_type _index;

    friend class Seq<T>;
  }; // class iterator
  using const_iterator = const iterator;

  class reverse_iterator : public iterator {
  public:
    always_inline reverse_iterator(memoir::Collection *const storage,
                                   size_type index)
      : iterator(storage, index) {}
    always_inline reverse_iterator(reverse_iterator &iter) : iterator(iter) {}

    always_inline reverse_iterator &operator--() {
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

  always_inline Seq(size_type n = 0)
    : Seq(memoir::MEMOIR_FUNC(allocate)(memoir_type<Seq<T>>, n)) {
    // Do nothing.
  }

  always_inline Seq(Ref<Seq<T>> seq) : collection(seq) {
    MEMOIR_FUNC(assert_type)
    (memoir_type<Seq<T>>, seq);
  }

  // Copy-constructor.
  always_inline Seq(const Seq &x)
    : Seq(MEMOIR_FUNC(sequence_copy)(x._storage, 0, x.size())) {}

  always_inline Ref<Seq<T>> operator&() const {
    return this->_storage;
  }

  // TODO
  always_inline void assign(size_type count, const T &value) {
    if (this->size() != count) {
      MEMOIR_FUNC(delete_collection)(this->_storage);
      this->_storage = MEMOIR_FUNC(allocate)(memoir_type<Seq<T>>, count);
    }
    for (std::size_t i = 0; i < count; ++i) {
      this[i] = value;
    }
  }

  // Element access.
  always_inline Element<T, size_type> operator[](size_type idx) {
    return Element<T, size_type>(this->_storage, idx);
  }

  always_inline const Element<T, size_type> operator[](size_type idx) const {
    return Element<T, size_type>(this->_storage, idx);
  }

  always_inline sequence_element at(size_type idx) const {
    if (idx < 0 || idx > this->size()) {
#if __cpp_extensions
      throw std::out_of_range("Seq.at() out of bounds");
#else
      abort();
#endif
    }
    return (*this)[idx];
  }

  always_inline sequence_element front() {
    return (*this)[0];
  }

  always_inline sequence_element back() {
    return (*this)[this->size() - 1];
  }

  // Iterators.
  always_inline iterator begin() {
    return iterator(this->_storage, 0);
  }

  always_inline iterator end() {
    return iterator(this->_storage, this->size());
  }

  always_inline const_iterator cbegin() const {
    return iterator(this->_storage, 0);
  }

  always_inline const_iterator cend() const {
    return iterator(this->_storage, this->size());
  }

  always_inline reverse_iterator rbegin() {
    return reverse_iterator(this->_storage, this->size() - 1);
  }

  always_inline reverse_iterator rend() {
    return reverse_iterator(this->_storage, -1);
  }

  always_inline const_reverse_iterator crbegin() const {
    return reverse_iterator(this->_storage, this->size() - 1);
  }

  always_inline const_reverse_iterator crend() const {
    return reverse_iterator(this->_storage, -1);
  }

  // Capacity.
  always_inline size_type size() const {
    return MEMOIR_FUNC(size)(this->_storage);
  }

  always_inline bool empty() const {
    return this->size() == 0;
  }

  // Modifiers.
  always_inline void insert(T value, size_type index) {
    if constexpr (std::is_pointer_v<T>) {
      using inner_type = typename std::remove_pointer_t<T>;
      if constexpr (std::is_base_of_v<inner_type, memoir::object>) {
        MUT_FUNC(sequence_insert_ref)(value, this->_storage, index);
      } else {
        MUT_FUNC(sequence_insert_ptr)(value, this->_storage, index);
      }
    }
#define HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)                            \
  else if constexpr (std::is_same_v<T, C_TYPE>) {                              \
    MUT_FUNC(sequence_insert_##TYPE_NAME)(value, this->_storage, index);       \
  }
#define HANDLE_INTEGER_TYPE(TYPE_NAME, C_TYPE, BW, IS_SIGNED)                  \
  HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)
#include <types.def>
#undef HANDLE_PRIMITIVE_TYPE
#undef HANDLE_INTEGER_TYPE
  }

  always_inline void push_back(T value) {
    if constexpr (std::is_pointer_v<T>) {
      using inner_type = typename std::remove_pointer_t<T>;
      if constexpr (std::is_base_of_v<inner_type, memoir::object>) {
        MUT_FUNC(sequence_insert_ref)
        (value, this->_storage, MEMOIR_FUNC(end)());
      } else {
        MUT_FUNC(sequence_insert_ptr)
        (value, this->_storage, MEMOIR_FUNC(end)());
      }
    }
#define HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)                            \
  else if constexpr (std::is_same_v<T, C_TYPE>) {                              \
    MUT_FUNC(sequence_insert_##TYPE_NAME)                                      \
    (value, this->_storage, MEMOIR_FUNC(end)());                               \
  }
#define HANDLE_INTEGER_TYPE(TYPE_NAME, C_TYPE, BW, IS_SIGNED)                  \
  HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)
#include <types.def>
#undef HANDLE_PRIMITIVE_TYPE
#undef HANDLE_INTEGER_TYPE
  }

  always_inline void push_back() {
    MUT_FUNC(sequence_insert)(this->_storage, MEMOIR_FUNC(end));
  }

  always_inline void insert(const Seq &to_insert, size_type index) {
    MUT_FUNC(sequence_insert)(to_insert._storage, this->_storage, index);
  }

  always_inline iterator insert(const_iterator pos, const T &value) {
    this->insert(pos._index, value);
    return iterator(this->_storage, pos._index);
  }

  always_inline iterator insert(const_iterator pos, T &&value) {
    this->insert(pos._index, value);
    return iterator(this->_storage, pos._index);
  }

  always_inline void remove(size_type from, size_type to) {
    MUT_FUNC(sequence_remove)(this->_storage, from, to);
  }

  always_inline void remove(size_type index) {
    MUT_FUNC(sequence_remove)(this->_storage, index, index + 1);
  }

  always_inline iterator erase(const_iterator pos) {
    if (pos._storage != this->_storage) {
      return pos;
    }
    if (pos == this->end()) {
      return pos;
    }
    this->remove(pos._index);
    return iterator(this->_storage, pos._index);
  }

  always_inline iterator erase(const_iterator first, const_iterator last) {
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

  always_inline void resize(size_type count) {
    if (count == this->size()) {
      return;
    }
    this->remove(count, this->size());
  }

  always_inline void resize(size_type count, const T &value) {
    this->assign(count, value);
  }

  // TODO
  // void swap(list<T> &other);

  always_inline void append(const Seq &to_append) {
    MEMOIR_FUNC(sequence_append)(this->_storage, to_append._storage);
  }

  always_inline void swap(size_type from_begin,
                          size_type from_end,
                          size_type to_begin) {
    MEMOIR_FUNC(sequence_swap)
    (this->_storage, from_begin, from_end, this->_storage, to_begin);
  }

  always_inline Seq split(size_type from, size_type to) {
    return Seq(MEMOIR_FUNC(sequence_split)(this->_storage, from, to));
  }

  always_inline Seq copy(size_type from, size_type to) {
    return Seq(MEMOIR_FUNC(sequence_slice)(this->_storage, from, to));
  }

  always_inline operator collection_ref() {
    return this->_storage;
  }

  always_inline Seq copy() {
    return this->copy(0, this->size());
  }

  always_inline void swap(Seq<T> &other) {
    auto *tmp = this->_storage;
    this->_storage = other._storage;
    other._storage = tmp;
  }

  always_inline void type() {
    MEMOIR_FUNC(assert_collection_type)(memoir_type<Seq<T>>, this->_storage);
  }

  template <typename RetTy, typename... Args>
  always_inline RetTy fold(RetTy init,
                           RetTy (*func)(RetTy, size_t, T, Args...),
                           Args... args) const {
    if (false) {
      // Stub.
    }
#define HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)                            \
  else if constexpr (std::is_same_v<T, C_TYPE>) {                              \
    return MEMOIR_FUNC(                                                        \
        fold_##TYPE_NAME)(init, this->_storage, (void *)func, args...);        \
  }
#define HANDLE_INTEGER_TYPE(TYPE_NAME, C_TYPE, BW, IS_SIGNED)                  \
  HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)
#include <types.def>
#undef HANDLE_PRIMITIVE_TYPE
#undef HANDLE_INTEGER_TYPE
  }
};

} // namespace memoir

#endif // MEMOIR_CPP_SEQUENCE_HH
