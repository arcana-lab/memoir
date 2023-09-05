#ifndef MEMOIR_CPP_ASSOC_HH
#define MEMOIR_CPP_ASSOC_HH
#pragma once

#include "memoir.h"

#include "sequence.hh"

namespace memoir {

/*
 * Accessing an assoced collection
 */
template <typename K, typename T>
class assoc {
  // static_assert(is_specialization<remove_all_pointers_t<T>, memoir::object>,
  //               "Trying to store non memoir object in a memoir collection!");

  class assoc_element {
  public:
    // using inner_type = typename std::remove_pointer_t<T>;

    assoc_element &operator=(assoc_element &&other) {
      this->target_object = std::move(other.target_object);
      this->key = std::move(other.key);
      return *this;
    }

    assoc_element &operator=(assoc_element other) {
      this->target_object = std::swap(this->target_object, other.target_object);
      this->key = std::swap(this->key, other.key);
      return *this;
    }

    T &operator->() {
      if constexpr (std::is_pointer_v<T>) {
        using inner_type = typename std::remove_pointer_t<T>;
        if constexpr (is_specialization<inner_type, memoir::object>) {
          return std::move(T(MEMOIR_FUNC(
              assoc_read_struct_ref)(this->target_object, this->key)));
        }
      }
    }

    T &operator=(T &&val) const {
      if constexpr (is_specialization<T, memoir::object>) {
        // TODO: copy construct the incoming struct
        // return object(
        //     MEMOIR_FUNC(assoc_get_struct_ref)(this->target_object,
        //     this->key));
        return val;
      } else if constexpr (std::is_pointer_v<T>) {
        using inner_type = typename std::remove_pointer_t<T>;
        if constexpr (is_specialization<inner_type, memoir::object>) {
          MEMOIR_FUNC(assoc_write_struct_ref)
          (val->target_object, this->target_object, this->key);
          return val;
        } else {
          MEMOIR_FUNC(assoc_write_ptr)
          (std::forward<T>(val), this->target_object, this->key);
          return val;
        }
      }
#define HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)                            \
  else if constexpr (std::is_same_v<T, C_TYPE>) {                              \
    MEMOIR_FUNC(assoc_write_##TYPE_NAME)                                       \
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
        using inner_type = typename std::remove_pointer_t<T>;
        if constexpr (is_specialization<inner_type, memoir::object>) {
          return object(MEMOIR_FUNC(assoc_read_struct_ref)(this->target_object,
                                                           this->key));
        } else {
          return object(
              MEMOIR_FUNC(assoc_read_ptr)(this->target_object, this->key));
        }
      } else if constexpr (is_specialization<T, memoir::object>) {
        return object(
            MEMOIR_FUNC(assoc_get_struct)(this->target_object, this->key));
      }
#define HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)                            \
  else if constexpr (std::is_same_v<T, C_TYPE>) {                              \
    return MEMOIR_FUNC(assoc_read_##TYPE_NAME)(this->target_object,            \
                                               this->key);                     \
  }
#define HANDLE_INTEGER_TYPE(TYPE_NAME, C_TYPE, BW, IS_SIGNED)                  \
  HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)
#include <types.def>
#undef HANDLE_PRIMITIVE_TYPE
#undef HANDLE_INTEGER_TYPE
    } // operator T()

    assoc_element(memoir::Collection *target_object, K key)
      : target_object(target_object),
        key(key) {
      // Do nothing.
    }

    assoc_element(memoir::Collection *target_object, K &&key)
      : target_object(target_object),
        key(key) {
      // Do nothing.
    }

    memoir::Collection *const target_object;
    const K key;
  }; // class assoc_element

public:
  assoc()
    : _storage(memoir::MEMOIR_FUNC(allocate_assoc_array)(
        primitive_type<K>::memoir_type,
        primitive_type<T>::memoir_type)) {
    // Do nothing.
  }

  assoc(memoir::Collection *storage) : _storage(storage) {
    // Do nothing.
  }

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

protected:
  memoir::Collection *const _storage;
}; // class assoc

} // namespace memoir

#endif // MEMOIR_CPP_ASSOC_HH
