#ifndef MEMOIR_CPP_ASSOC_HH
#define MEMOIR_CPP_ASSOC_HH
#pragma once

#include "memoir.h"

#include "collection.hh"
#include "object.hh"
#include "sequence.hh"

namespace memoir {

/*
 * Accessing an assoced collection
 */
template <typename K, typename T>
class assoc : public collection {
  // static_assert(is_specialization<remove_all_pointers_t<T>, memoir::object>,
  //               "Trying to store non memoir object in a memoir collection!");
public:
  using key_type = K;
  using value_type = T;
  using difference_type = std::ptrdiff_t;

protected:
  class object_assoc_element : public T {
  public:
    object_assoc_element &operator=(object_assoc_element &&other) {
      this->target_object = std::move(other.target_object);
      this->key = std::move(other.key);
      return *this;
    }

    object_assoc_element &operator=(object_assoc_element other) {
      this->target_object = std::swap(this->target_object, other.target_object);
      this->key = std::swap(this->key, other.key);
      return *this;
    }

    T &operator->() {
      if constexpr (std::is_pointer_v<T>) {
        using inner_type = std::remove_pointer_t<T>;
        if constexpr (std::is_base_of_v<memoir::object, inner_type>) {
          return std::move(T(MEMOIR_FUNC(
              assoc_read_struct_ref)(this->target_object, this->key)));
        }
      }
    }

    T &operator=(T &&val) const {
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

    operator T() const {
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

    object_assoc_element(memoir::Collection *target_object, K key)
      : T(MEMOIR_FUNC(assoc_get_struct)(target_object, key)),
        target_object(target_object),
        key(key) {
      // Do nothing.
    }

    object_assoc_element(memoir::Collection *target_object, K &&key)
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

    primitive_assoc_element &operator=(primitive_assoc_element &&other) {
      this->target_object = std::move(other.target_object);
      this->key = std::move(other.key);
      return *this;
    }

    primitive_assoc_element &operator=(primitive_assoc_element other) {
      this->target_object = std::swap(this->target_object, other.target_object);
      this->key = std::swap(this->key, other.key);
      return *this;
    }

    T &operator=(T &&val) const {
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
  HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)
#include <types.def>
#undef HANDLE_PRIMITIVE_TYPE
#undef HANDLE_INTEGER_TYPE
#warning "Unsupported type being assigned to assoc contents"
    } // T &operator=(T &&)

    operator T() const {
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

    primitive_assoc_element(memoir::Collection *target_object, K key)
      : target_object(target_object),
        key(key) {
      // Do nothing.
    }

    primitive_assoc_element(memoir::Collection *target_object, K &&key)
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
    using key_iterator = typename sequence<K>::iterator;

    // Constructors.
    iterator(memoir::Collection *storage, const key_type &k)
      : _storage(storage),
        _key(k),
        _key_iterator({}) {}
    iterator(memoir::Collection *storage, key_iterator key_iterator)
      : _storage(storage),
        _key_iterator(key_iterator) {}
    iterator(memoir::Collection *storage,
             memoir::Collection *keys,
             difference_type index)
      : iterator(storage, key_iterator(keys, index)) {}
    iterator(memoir::Collection *storage, memoir::Collection *keys)
      : iterator(storage, key_iterator(keys, 0)) {}
    iterator(memoir::Collection *storage, difference_type index)
      : iterator(
          storage,
          sequence<K>::iterator(MEMOIR_FUNC(assoc_keys)(storage), index)) {}
    iterator(memoir::Collection *storage)
      : iterator(storage, (difference_type)0) {}
    iterator(iterator &iter) : iterator(iter._storage, iter._key_iterator) {}

    // Splat.
    reference operator*() const {
      return assoc_element(this->_storage, *_key_iterator);
    }

    pointer operator->() const {
      return assoc_element(this->_storage, *_key_iterator);
    }

    // Prefix increment.
    iterator &operator++() {
      ++this->_key_iterator;
      return (*this);
    }

    // Postfix increment.
    iterator operator++(int) {
      iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    // Equality.
    friend bool operator==(const iterator &a, const iterator &b) {
      return (a._storage == b.storage) && (a._key_iterator == b._key_iterator);
    }

    friend bool operator==(const iterator &a, const iterator &b) {
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
    using reverse_key_iterator = typename sequence<K>::reverse_iterator;

    // Constructors.
    reverse_iterator(memoir::Collection *storage,
                     memoir::Collection *keys,
                     difference_type index)
      : iterator(storage, reverse_key_iterator(keys, index)) {}
    reverse_iterator(memoir::Collection *storage, memoir::Collection *keys)
      : reverse_iterator(storage, keys, MEMOIR_FUNC(size)(storage) - 1) {}
    reverse_iterator(memoir::Collection *storage)
      : reverse_iterator(storage, MEMOIR_FUNC(assoc_keys)(storage)) {}
    reverse_iterator(reverse_iterator &iter)
      : reverse_iterator(iter._storage, iter._keys, iter._index) {}

    // Prefix decrement.
    reverse_iterator &operator--() {
      --this->_key_iterator;
      return *this;
    }

    // Postfix decrement.
    reverse_iterator operator--(int) {
      reverse_iterator tmp = *this;
      --(*this);
      return tmp;
    }
  };
  using const_reverse_iterator = const reverse_iterator;

  assoc()
    : assoc(memoir::MEMOIR_FUNC(allocate_assoc_array)(to_memoir_type<K>(),
                                                      to_memoir_type<T>())) {
    // Do nothing.
  }

  assoc(memoir::Collection *storage) : collection(storage) {
    // Do nothing.
  }

  // Element access.
  assoc_element operator[](K &&key) {
    return assoc_element(this->_storage, key);
  }

  assoc_element operator[](K &&key) const {
    return assoc_element(this->_storage, key);
  }

  bool has(K &&key) const {
    return MEMOIR_FUNC(assoc_has)(this->_storage, key);
  }

  void remove(std::size_t key) const {
    MEMOIR_FUNC(assoc_remove)(this->_storage, key);
  }

  sequence<K> keys() const {
    return sequence<K>(MEMOIR_FUNC(assoc_keys)(this->_storage));
  }
}; // class assoc

} // namespace memoir

#endif // MEMOIR_CPP_ASSOC_HH
