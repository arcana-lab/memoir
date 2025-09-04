#ifndef MEMOIR_CPP_ASSOC_HH
#define MEMOIR_CPP_ASSOC_HH
#pragma once

#include "memoir.h"

#include "memoir++/collection.hh"
#include "memoir++/object.hh"
#include "memoir++/sequence.hh"

namespace memoir {

/*
 * Accessing an associative collection
 */
template <typename T>
class Set : public collection {
  // static_assert(is_specialization<remove_all_pointers_t<T>, memoir::object>,
  //               "Trying to store non memoir object in a memoir collection!");
public:
  using key_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

protected:
public:
  class iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = value_type;
    using reference = value_type;
    using key_iterator = typename Seq<T>::iterator;

    // Constructors.
    always_inline iterator(memoir::Collection *storage, const key_type &k)
      : _storage(storage),
        _key(k),
        _key_iterator({}) {}
    always_inline iterator(memoir::Collection *storage,
                           key_iterator key_iterator)
      : _storage(storage),
        _key_iterator(key_iterator) {}
    always_inline iterator(memoir::Collection *storage,
                           memoir::Collection *keys,
                           difference_type index)
      : iterator(storage, key_iterator(keys, index)) {}
    always_inline iterator(memoir::Collection *storage,
                           memoir::Collection *keys)
      : iterator(storage, key_iterator(keys, 0)) {}
    always_inline iterator(memoir::Collection *storage, difference_type index)
      : iterator(storage,
                 Seq<T>::iterator(MEMOIR_FUNC(assoc_keys)(storage), index)) {}
    always_inline iterator(memoir::Collection *storage)
      : iterator(storage, (difference_type)0) {}
    always_inline iterator(iterator &iter)
      : iterator(iter._storage, iter._key_iterator) {}

    // Splat.
    always_inline reference operator*() const {
      return assoc_element(this->_storage, *_key_iterator);
    }

    always_inline pointer operator->() const {
      return assoc_element(this->_storage, *_key_iterator);
    }

    // Prefix increment.
    always_inline iterator &operator++() {
      ++this->_key_iterator;
      return (*this);
    }

    // Postfix increment.
    always_inline iterator operator++(int) {
      iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    // Equality.
    always_inline friend bool operator==(const iterator &a, const iterator &b) {
      return (a._storage == b.storage) && (a._key_iterator == b._key_iterator);
    }

    always_inline friend bool operator==(const iterator &a, const iterator &b) {
      return (a._storage != b.storage) || (a._key_iterator != b._key_iterator);
    }

  protected:
    memoir::Collection *const _storage;
    const key_type _key;
    key_iterator _key_iterator;
  };
  using const_iterator = const iterator;

  class reverse_iterator : public iterator {
  public:
    using reverse_key_iterator = typename Seq<T>::reverse_iterator;

    // Constructors.
    always_inline reverse_iterator(memoir::Collection *storage,
                                   memoir::Collection *keys,
                                   difference_type index)
      : iterator(storage, reverse_key_iterator(keys, index)) {}
    always_inline reverse_iterator(memoir::Collection *storage,
                                   memoir::Collection *keys)
      : reverse_iterator(storage, keys, MEMOIR_FUNC(size)(storage) - 1) {}
    always_inline reverse_iterator(memoir::Collection *storage)
      : reverse_iterator(storage, MEMOIR_FUNC(assoc_keys)(storage)) {}
    always_inline reverse_iterator(reverse_iterator &iter)
      : reverse_iterator(iter._storage, iter._keys, iter._index) {}

    // Prefix decrement.
    always_inline reverse_iterator &operator--() {
      --this->_key_iterator;
      return *this;
    }

    // Postfix decrement.
    always_inline reverse_iterator operator--(int) {
      reverse_iterator tmp = *this;
      --(*this);
      return tmp;
    }
  };
  using const_reverse_iterator = const reverse_iterator;

  always_inline Set()
    : Set(memoir::MEMOIR_FUNC(allocate_assoc_array)(memoir_type<T>,
                                                    memoir_type<void>)) {
    // Do nothing.
  }

  // Copy-constructor.
  // always_inline Seq(const Seq &x) : Seq(MEMOIR_FUNC(copy)(x._storage)) {}

  // Move-constructor.
  always_inline Set(Ref<Set<T>> ref) : collection(ref) {
    MEMOIR_FUNC(assert_collection_type)
    (memoir_type<Set<T>>, ref);
  }

  always_inline Ref<Set<T>> operator&() const {
    return this->_storage;
  }

  always_inline bool has(const T &key) const {
    return MEMOIR_FUNC(assoc_has)(this->_storage, key);
  }

  always_inline void remove(const T &key) const {
    MUT_FUNC(assoc_remove)(this->_storage, key);
  }

  always_inline void insert(const T &key) const {
    MUT_FUNC(assoc_insert)(this->_storage, key);
  }

  always_inline Seq<T> keys() const {
    return Seq<T>(MEMOIR_FUNC(assoc_keys)(this->_storage));
  }

  always_inline size_type size() const {
    return MEMOIR_FUNC(size)(this->_storage);
  }

  always_inline void swap(Set<T> &other) {
    auto *tmp = this->_storage;
    this->_storage = other._storage;
    other._storage = tmp;
  }

  template <typename RetTy, typename... Args>
  always_inline RetTy fold(RetTy init,
                           RetTy (*func)(RetTy, T, Args...),
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

}; // class Set

} // namespace memoir

#endif // MEMOIR_CPP_ASSOC_HH
