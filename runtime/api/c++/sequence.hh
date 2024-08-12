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
class Seq : public collection {
  // static_assert(is_instance_of_v<remove_all_pointers_t<T>, memoir::object>,
  //               "Trying to store non memoir object in a memoir collection!");
public:
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

protected:
  class object_sequence_element : public T {
  public:
    // using inner_type = typename std::remove_pointer_t<T>;

    // object_sequence_element &operator=(object_sequence_element &&other) {
    //   this->target_object = std::move(other.target_object);
    //   this->idx = std::move(other.idx);
    //   return *this;
    // }

    // object_sequence_element &operator=(object_sequence_element other) {
    //   this->target_object = std::swap(this->target_object,
    //   other.target_object); this->idx = std::swap(this->idx, other.idx);
    //   return *this;
    // }

    always_inline T &operator=(T &&val) const {
      if constexpr (std::is_base_of_v<memoir::object, T>) {
        // TODO: copy construct the incoming struct
        return val;
      } else if constexpr (std::is_pointer_v<T>) {
        using inner_type = typename std::remove_pointer_t<T>;
        if constexpr (std::is_base_of_v<memoir::object, inner_type>) {
          MUT_FUNC(index_write_struct_ref)
          (val->target_object, this->target_object, this->idx);
          return val;
        }
      }
    } // T &operator=(T &&)

    always_inline operator T() const {
      MEMOIR_FUNC(assert_collection_type)
      (memoir_type<Seq<T>>, this->target_object);
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

    always_inline T operator*() const {
      MEMOIR_FUNC(assert_collection_type)
      (memoir_type<Seq<T>>, this->target_object);
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
    always_inline object_sequence_element(memoir::Collection *target_object,
                                          size_type idx)
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

    // collection_sequence_element &operator=(
    //     collection_sequence_element &&other) {
    //   this->target_object = std::move(other.target_object);
    //   this->idx = std::move(other.idx);
    //   return *this;
    // }

    // collection_sequence_element &operator=(collection_sequence_element other)
    // {
    //   this->target_object = std::swap(this->target_object,
    //   other.target_object); this->idx = std::swap(this->idx, other.idx);
    //   return *this;
    // }

    always_inline T &operator=(T &&val) const {
      if constexpr (std::is_base_of_v<memoir::collection, T>) {
        // TODO: copy construct the incoming collection
        return val;
      } else if constexpr (std::is_pointer_v<T>) {
        using inner_type = typename std::remove_pointer_t<T>;
        if constexpr (std::is_base_of_v<memoir::collection, inner_type>) {
          MUT_FUNC(index_write_collection_ref)
          (val->target_object, this->target_object, this->idx);
          return val;
        }
      }
    } // T &operator=(T &&)

    always_inline operator T() const {
      MEMOIR_FUNC(assert_collection_type)
      (memoir_type<Seq<T>>, this->target_object);

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

    always_inline T operator*() const {
      MEMOIR_FUNC(assert_collection_type)
      (memoir_type<Seq<T>>, this->target_object);

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

    always_inline collection_sequence_element(memoir::Collection *target_object,
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

    // primitive_sequence_element &operator=(primitive_sequence_element &&other)
    // {
    //   this->target_object = std::move(other.target_object);
    //   this->idx = std::move(other.idx);
    //   return *this;
    // }

    // primitive_sequence_element &operator=(primitive_sequence_element other) {
    //   this->target_object = std::swap(this->target_object,
    //   other.target_object); this->idx = std::swap(this->idx, other.idx);
    //   return *this;
    // }

    always_inline T operator=(T val) const {
      MEMOIR_FUNC(assert_collection_type)
      (memoir_type<Seq<T>>, this->target_object);

      if constexpr (std::is_pointer_v<T>) {
        MUT_FUNC(index_write_ptr)
        (val, this->target_object, this->idx);
        return val;
      }
#define HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)                            \
  else if constexpr (std::is_same_v<T, C_TYPE>) {                              \
    MUT_FUNC(index_write_##TYPE_NAME)                                          \
    (val, this->target_object, this->idx);                                     \
    return val;                                                                \
  }
#define HANDLE_INTEGER_TYPE(TYPE_NAME, C_TYPE, BW, IS_SIGNED)                  \
  HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)
#include <types.def>
#undef HANDLE_PRIMITIVE_TYPE
#undef HANDLE_INTEGER_TYPE
    } // T &operator=(T &&)

    always_inline operator T() const {
      MEMOIR_FUNC(assert_collection_type)
      (memoir_type<Seq<T>>, this->target_object);

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

    always_inline T operator*() const {
      MEMOIR_FUNC(assert_collection_type)
      (memoir_type<Seq<T>>, this->target_object);

      if constexpr (std::is_pointer_v<T>) {
        return MEMOIR_FUNC(index_read_ptr)(this->target_object, this->idx);
      }
#define HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)                            \
  else if constexpr (std::is_same_v<T, C_TYPE>) {                              \
    return MEMOIR_FUNC(index_read_##TYPE_NAME)(this->target_object,            \
                                               this->idx);                     \
  }
#define HANDLE_INTEGER_TYPE(TYPE_NAME, C_TYPE, BW, IS_SIGNED)                  \
  else if constexpr (std::is_same_v<T, C_TYPE>) {                              \
    return MEMOIR_FUNC(index_read_##TYPE_NAME)(this->target_object,            \
                                               this->idx);                     \
  }
#include <types.def>
#undef HANDLE_PRIMITIVE_TYPE
#undef HANDLE_INTEGER_TYPE
      else {
        throw std::runtime_error("unknown Seq element type");
      }
    } // operator T()

    always_inline primitive_sequence_element(memoir::Collection *target_object,
                                             size_type idx)
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
    always_inline iterator(memoir::Collection *const storage,
                           difference_type index)
      : _storage(storage),
        _index(index) {}

    always_inline iterator(iterator &elem)
      : _storage(elem._storage),
        _index(elem._index) {}

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

    always_inline friend bool operator==(const iterator &a, const iterator &b) {
      return (a._storage == b._storage) && (a._index == b._index);
    }

    always_inline friend bool operator!=(const iterator &a, const iterator &b) {
      return (a._storage != b._storage) || (a._index != b._index);
    }

  protected:
    memoir::Collection *const _storage;
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
    : Seq(memoir::MEMOIR_FUNC(allocate_sequence)(memoir_type<T>, n)) {
    // Do nothing.
  }

  always_inline Seq(Ref<Seq<T>> seq) : collection(seq) {
    MEMOIR_FUNC(assert_collection_type)
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
      this->_storage = MEMOIR_FUNC(allocate_sequence)(memoir_type<T>, count);
    }
    for (std::size_t i = 0; i < count; ++i) {
      this[i] = value;
    }
  }

  // Element access.
  always_inline sequence_element operator[](size_type idx) {
    return sequence_element(this->_storage, idx);
  }

  always_inline sequence_element operator[](size_type idx) const {
    return sequence_element(this->_storage, idx);
  }

  always_inline sequence_element at(size_type idx) const {
    if (idx < 0 || idx > this->size()) {
      throw std::out_of_range("Seq.at() out of bounds");
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
  always_inline void clear() {
    MEMOIR_FUNC(sequence_remove)(this->_storage, 0, this->size());
  }

  always_inline void insert(T value, size_type index) {
    if constexpr (std::is_pointer_v<T>) {
      using inner_type = typename std::remove_pointer_t<T>;
      if constexpr (std::is_base_of_v<inner_type, memoir::object>) {
        MUT_FUNC(sequence_insert_struct_ref)(value, this->_storage, index);
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

  always_inline Seq copy() {
    return this->copy(0, this->size());
  }
}; // namespace memoir

} // namespace memoir

#endif // MEMOIR_CPP_SEQUENCE_HH
