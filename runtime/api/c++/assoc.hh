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
template <typename K, typename T>
class Assoc : public collection {
  // static_assert(is_specialization<remove_all_pointers_t<T>, memoir::object>,
  //               "Trying to store non memoir object in a memoir collection!");
public:
  using key_type = K;
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

protected:
  class object_assoc_element : public T {
  public:
    // always_inline object_assoc_element &operator=(
    //     object_assoc_element &&other) {
    //   this->target_object = std::move(other.target_object);
    //   this->key = std::move(other.key);
    //   return *this;
    // }

    // always_inline object_assoc_element &operator=(object_assoc_element other)
    // {
    //   this->target_object = std::swap(this->target_object,
    //   other.target_object); this->key = std::swap(this->key, other.key);
    //   return *this;
    // }

    always_inline T &operator->() {
      if constexpr (std::is_pointer_v<T>) {
        using inner_type = std::remove_pointer_t<T>;
        if constexpr (std::is_base_of_v<memoir::object, inner_type>) {
          return std::move(T(MEMOIR_FUNC(
              assoc_read_struct_ref)(this->target_object, this->key)));
        }
      }
    }

    always_inline T &operator=(T &&val) const {
      if constexpr (std::is_base_of_v<memoir::object, T>) {
        // TODO: copy construct the incoming struct
        return val;
      } else if constexpr (std::is_pointer_v<T>) {
        using inner_type = std::remove_pointer_t<T>;
        if constexpr (std::is_base_of_v<memoir::object, inner_type>) {
          MUT_FUNC(assoc_write_struct_ref)
          (val->target_object, this->target_object, this->key);
          return val;
        }
      }
    } // T &operator=(T &&)

    always_inline operator T() const {
      if constexpr (std::is_pointer_v<T>) {
        using inner_type = typename std::remove_pointer_t<T>;
        if constexpr (std::is_base_of_v<memoir::object, inner_type>) {
          return object(MEMOIR_FUNC(assoc_read_struct_ref)(this->target_object,
                                                           this->key));
        } else {
          return object(
              MEMOIR_FUNC(assoc_read_ptr)(this->target_object, this->key));
        }
      } else if constexpr (std::is_base_of_v<memoir::object, T>) {
        return object(
            MEMOIR_FUNC(assoc_get_struct)(this->target_object, this->key));
      }
    } // operator T()

    always_inline object_assoc_element(memoir::Collection *target_object, K key)
      : T(MEMOIR_FUNC(assoc_get_struct)(target_object, key)),
        target_object(target_object),
        key(key) {
      // Do nothing.
    }

    always_inline object_assoc_element(memoir::Collection *target_object,
                                       K &&key)
      : T(MEMOIR_FUNC(assoc_get_struct)(target_object, key)),
        target_object(target_object),
        key(key) {
      // Do nothing.
    }

    memoir::Collection *const target_object;
    const K key;
  }; // class object_assoc_element

  class primitive_assoc_element {
  public:
    // using inner_type = typename std::remove_pointer_t<T>;

    // primitive_assoc_element &operator=(primitive_assoc_element &&other) {
    //     this->target_object = std::move(other.target_object);
    //     this->key = std::move(other.key);
    //     return *this;
    // }

    // primitive_assoc_element
    // &operator=(primitive_assoc_element other) {
    //     this->target_object = std::swap(this->target_object,
    //     other.target_object); this->key = std::swap(this->key, other.key);
    //     return *this;
    // }

    always_inline T &operator=(T &&val) const {
      if constexpr (std::is_pointer_v<T>) {
        MUT_FUNC(assoc_write_ptr)(val, this->target_object, this->key);
        return val;
      }
#define HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)                            \
  else if constexpr (std::is_same_v<T, C_TYPE>) {                              \
    MUT_FUNC(assoc_write_##TYPE_NAME)                                          \
    (val, this->target_object, this->key);                                     \
    return val;                                                                \
  }
#define HANDLE_INTEGER_TYPE(TYPE_NAME, C_TYPE, BW, IS_SIGNED)                  \
  else if constexpr (std::is_same_v<T, C_TYPE>) {                              \
    MUT_FUNC(assoc_write_##TYPE_NAME)                                          \
    (val, this->target_object, this->key);                                     \
    return val;                                                                \
  }
#include <types.def>
#undef HANDLE_PRIMITIVE_TYPE
#undef HANDLE_INTEGER_TYPE
    } // T &operator=(T &&)

    always_inline operator T() const {
      if constexpr (std::is_pointer_v<T>) {
        return object(
            MEMOIR_FUNC(assoc_read_ptr)(this->target_object, this->key));
      }
#define HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)                            \
  else if constexpr (std::is_same_v<T, C_TYPE>) {                              \
    return MEMOIR_FUNC(assoc_read_##TYPE_NAME)(this->target_object,            \
                                               this->key);                     \
  }
#define HANDLE_INTEGER_TYPE(TYPE_NAME, C_TYPE, BW, IS_SIGNED)                  \
  HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)
#include <types.def>
    } // operator T()

    always_inline primitive_assoc_element(memoir::Collection *target_object,
                                          K key)
      : target_object(target_object),
        key(key) {
      // Do nothing.
    }

    always_inline primitive_assoc_element(memoir::Collection *target_object,
                                          K &&key)
      : target_object(target_object),
        key(key) {
      // Do nothing.
    }

    memoir::Collection *const target_object;
    const K key;
  }; // class primitive_assoc_element

  using assoc_element = std::conditional_t<
      std::is_base_of_v<memoir::object, std::remove_pointer_t<T>>,
      object_assoc_element,
      primitive_assoc_element>;

public:
  class iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = assoc_element;
    using pointer = value_type;
    using reference = value_type;
    using key_iterator = typename Seq<K>::iterator;

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
                 Seq<K>::iterator(MEMOIR_FUNC(assoc_keys)(storage), index)) {}
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
    using reverse_key_iterator = typename Seq<K>::reverse_iterator;

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

  always_inline Assoc()
    : Assoc(memoir::MEMOIR_FUNC(allocate_assoc_array)(memoir_type<K>,
                                                      memoir_type<T>)) {
    // Do nothing.
  }

  // Copy-constructor.
  // always_inline Seq(const Seq &x) : Seq(MEMOIR_FUNC(copy)(x._storage)) {}

  // Move-constructor.
  always_inline Assoc(Ref<Assoc<K, T>> ref) : collection(ref) {
    MEMOIR_FUNC(assert_collection_type)
    (memoir_type<Assoc<K, T>>, ref);
  }

  always_inline Ref<Assoc<K, T>> operator&() const {
    return this->_storage;
  }

  // Element access.
  always_inline assoc_element operator[](const K &key) {
    if (!this->has(key)) {
      MUT_FUNC(assoc_insert)(this->_storage, key);
    }
    return assoc_element(this->_storage, key);
  }

  always_inline assoc_element operator[](const K &key) const {
    return assoc_element(this->_storage, key);
  }

  always_inline bool has(const K &key) const {
    return MEMOIR_FUNC(assoc_has)(this->_storage, key);
  }

  always_inline void remove(const K &key) const {
    MUT_FUNC(assoc_remove)(this->_storage, key);
  }

  always_inline Seq<K> keys() const {
    return Seq<K>(MEMOIR_FUNC(assoc_keys)(this->_storage));
  }

  always_inline size_type size() const {
    return MEMOIR_FUNC(size)(this->_storage);
  }

  always_inline void swap(Assoc<K, T> &other) {
    auto *tmp = this->_storage;
    this->_storage = other._storage;
    other._storage = tmp;
  }

  template <typename RetTy, typename... Args>
  always_inline RetTy fold(RetTy init,
                           RetTy (*func)(RetTy, K, T, Args...),
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
}; // class Assoc

} // namespace memoir

#endif // MEMOIR_CPP_ASSOC_HH
