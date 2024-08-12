#ifndef MEMOIR_CPP_OBJECT_HH
#define MEMOIR_CPP_OBJECT_HH
#pragma once

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <list>
#include <unordered_set>

#include "memoir++/counter.hh"

#include <memoir.h>

namespace memoir {

// Stub types.
template <typename T>
class Seq;

template <typename K, typename T>
class Assoc;

/*
 * Helper types and functions.
 */
template <typename T>
struct identity {
  using type = T;
};

template <typename T>
struct remove_all_pointers
  : std::conditional_t<std::is_pointer_v<T>,
                       remove_all_pointers<std::remove_pointer_t<T>>,
                       identity<T>> {};

template <typename T>
using remove_all_pointers_t = typename remove_all_pointers<T>::type;

// template <typename T, template <typename...> class Template>
// constexpr bool is_specialization{ false };

// template <template <typename...> class Template, typename... Args>
// constexpr bool is_specialization<Template<Args...>, Template>{ true };

template <class T, template <class...> class Template>
inline constexpr bool is_instance_of_v = std::false_type{};

template <template <class...> class Template, class... Args>
inline constexpr bool is_instance_of_v<Template<Args...>, Template> =
    std::true_type{};

// Pretty wrapper for collection references.
template <typename T>
using Ref = collection_ref;

// Object interface.
class object {
public:
  always_inline object(memoir::Struct *storage) : _storage(storage) {}

  template <typename F, std::size_t field_index>
  struct field {
    always_inline field(memoir::Struct *const storage) : _storage(storage) {}

    always_inline operator F() const {
      if constexpr (std::is_pointer_v<F>) {
        using inner_type = typename std::remove_pointer_t<F>;
        if constexpr (std::is_base_of_v<typename memoir::object, inner_type>) {
          return F(memoir::MEMOIR_FUNC(struct_read_struct_ref)(this->_storage,
                                                               field_index));
        } else {
          return memoir::MEMOIR_FUNC(struct_read_ptr)(this->_storage,
                                                      field_index);
        }
      } else if constexpr (std::is_base_of_v<typename memoir::object, F>) {
        return F(memoir::MEMOIR_FUNC(struct_get_struct)(this->_storage,
                                                        field_index));
      }
#define HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)                            \
  else if constexpr (std::is_same_v<F, C_TYPE>) {                              \
    return memoir::MEMOIR_FUNC(struct_read_##TYPE_NAME)(this->_storage,        \
                                                        field_index);          \
  }
#define HANDLE_INTEGER_TYPE(TYPE_NAME, C_TYPE, BW, IS_SIGNED)                  \
  HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)
#include "types.def"
#undef HANDLE_PRIMITIVE_TYPE
#undef HANDLE_INTEGER_TYPE
    } // operator F()

    always_inline F operator=(F f) const {
      if constexpr (std::is_pointer_v<F>) {
        using inner_type = typename std::remove_pointer_t<F>;
        if constexpr (std::is_base_of_v<typename memoir::object, inner_type>) {
          memoir::MEMOIR_FUNC(struct_write_struct_ref)(f->_storage,
                                                       this->_storage,
                                                       field_index);
          return f;
        } else {
          memoir::MEMOIR_FUNC(struct_read_ptr)(f, this->_storage, field_index);
          return f;
        }
      } else if constexpr (std::is_base_of_v<typename memoir::object, F>) {
        // TODO: copy construct the struct
        return f;
      }
#define HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)                            \
  else if constexpr (std::is_same_v<F, C_TYPE>) {                              \
    memoir::MEMOIR_FUNC(                                                       \
        struct_write_##TYPE_NAME)(f, this->_storage, field_index);             \
    return f;                                                                  \
  }
#define HANDLE_INTEGER_TYPE(TYPE_NAME, C_TYPE, BW, IS_SIGNED)                  \
  HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)
#include "types.def"
#undef HANDLE_PRIMITIVE_TYPE
#undef HANDLE_INTEGER_TYPE
    }

    always_inline F operator*() const {
      if constexpr (std::is_pointer_v<F>) {
        using inner_type = typename std::remove_pointer_t<F>;
        if constexpr (std::is_base_of_v<typename memoir::object, inner_type>) {
          return F(memoir::MEMOIR_FUNC(struct_read_struct_ref)(this->_storage,
                                                               field_index));
        } else {
          return memoir::MEMOIR_FUNC(struct_read_ptr)(this->_storage,
                                                      field_index);
        }
      } else if constexpr (std::is_base_of_v<typename memoir::object, F>) {
        return F(memoir::MEMOIR_FUNC(struct_get_struct)(this->_storage,
                                                        field_index));
      }
    }

    memoir::Struct *const _storage;
  };

protected:
  memoir::Struct *const _storage;
};

// Create define_struct_type.
// template <typename T>
// const type_ref memoir_type;
template <typename T>
const type_ref memoir_type = nullptr;

template <typename T>
const type_ref memoir_type<Seq<T>> =
    memoir::MEMOIR_FUNC(sequence_type)(memoir_type<T>);

template <typename K, typename V>
const type_ref memoir_type<Assoc<K, V>> =
    MEMOIR_FUNC(assoc_array_type)(memoir_type<K>, memoir_type<V>);

template <typename T>
const type_ref memoir_type<T *> = MEMOIR_FUNC(ptr_type)();

template <>
const type_ref memoir_type<uint64_t> = MEMOIR_FUNC(u64_type)();
template <>
const type_ref memoir_type<uint32_t> = MEMOIR_FUNC(u32_type)();
template <>
const type_ref memoir_type<uint16_t> = MEMOIR_FUNC(u16_type)();
template <>
const type_ref memoir_type<uint8_t> = MEMOIR_FUNC(u8_type)();
template <>
const type_ref memoir_type<int64_t> = MEMOIR_FUNC(i64_type)();
template <>
const type_ref memoir_type<int32_t> = MEMOIR_FUNC(i32_type)();
template <>
const type_ref memoir_type<int16_t> = MEMOIR_FUNC(i16_type)();
template <>
const type_ref memoir_type<int8_t> = MEMOIR_FUNC(i8_type)();
template <>
const type_ref memoir_type<float> = MEMOIR_FUNC(f32_type)();
template <>
const type_ref memoir_type<double> = MEMOIR_FUNC(f64_type)();
template <>
const type_ref memoir_type<bool> = MEMOIR_FUNC(boolean_type)();

} // namespace memoir

// These macros let you set up a memoir struct, with both the C++ struct and the
// refl-cpp macro code.
//
// Example:
//   AUTO_STRUCT(
//     MyStruct,
//     FIELD(uint64_t, a),
//     FIELD(double, b),
//     FIELD(MyStruct *, ptr)
//   )

#define SEMICOLON_DELIM() ;
#define COMMA_DELIM() ,

#define TO_CPP_FIELD(FIELD_TYPE, FIELD_NAME) FIELD_TYPE FIELD_NAME

#define TO_CPP_PREPEND_(F) TO_CPP_##F
#define TO_CPP_PREPEND(F) TO_CPP_PREPEND_(F)

#define TO_CPP_STRUCT(NAME, FIELDS...)                                         \
  namespace memoir::user {                                                     \
  struct NAME {                                                                \
    MEMOIR_apply_delim(TO_CPP_PREPEND, SEMICOLON_DELIM, FIELDS);               \
  };                                                                           \
  }

// Create initializer list.
#define TO_INIT_FIELD(FIELD_TYPE, FIELD_NAME) FIELD_NAME(obj)

#define TO_INIT_PREPEND_(F) TO_INIT_##F
#define TO_INIT_PREPEND(F) TO_INIT_PREPEND_(F)

// Create fields.
#define TO_MEMOIR_FIELD(FIELD_TYPE, FIELD_NAME)                                \
  const memoir::object::field<FIELD_TYPE, field_index.next<__COUNTER__>()>     \
      FIELD_NAME

#define TO_MEMOIR_PREPEND_(F) TO_MEMOIR_##F
#define TO_MEMOIR_PREPEND(F) TO_MEMOIR_PREPEND_(F)

// Create initialization arguments.
#define TO_ARGS_FIELD(FIELD_TYPE, FIELD_NAME) const FIELD_TYPE &_##FIELD_NAME

#define TO_ARGS_PREPEND_(F) TO_ARGS_##F
#define TO_ARGS_PREPEND(F) TO_ARGS_PREPEND_(F)

// Create initialization assignments.
#define TO_FIELD_INIT_FIELD(FIELD_TYPE, FIELD_NAME)                            \
  this->FIELD_NAME = _##FIELD_NAME

#define TO_FIELD_INIT_PREPEND_(F) TO_FIELD_INIT_##F
#define TO_FIELD_INIT_PREPEND(F) TO_FIELD_INIT_PREPEND_(F)

// Create type.
#define TO_TYPE_FIELD(FIELD_TYPE, FIELD_NAME) memoir_type<FIELD_TYPE>

#define TO_TYPE_PREPEND_(F) TO_TYPE_##F
#define TO_TYPE_PREPEND(F) TO_TYPE_PREPEND_(F)

#define TO_MEMOIR_STRUCT(NAME, FIELDS...)                                      \
  class NAME : public memoir::object {                                         \
  public:                                                                      \
    always_inline NAME(memoir::Struct *obj)                                    \
      : object(obj),                                                           \
        MEMOIR_apply_delim(TO_INIT_PREPEND, COMMA_DELIM, FIELDS) {}            \
    always_inline NAME() : NAME(MEMOIR_FUNC(allocate_struct)(NAME::_type)) {}  \
    always_inline NAME(MEMOIR_apply_delim(TO_ARGS_PREPEND,                     \
                                          COMMA_DELIM,                         \
                                          FIELDS))                             \
      : NAME() {                                                               \
      MEMOIR_apply_delim(TO_FIELD_INIT_PREPEND, SEMICOLON_DELIM, FIELDS);      \
    }                                                                          \
    /* Instantiate field members. */                                           \
    constexpr static fameta::counter<fameta::context<__COUNTER__>, 0, 1>       \
        field_index;                                                           \
    MEMOIR_apply_delim(TO_MEMOIR_PREPEND, SEMICOLON_DELIM, FIELDS);            \
                                                                               \
    static memoir::Type *const _type;                                          \
  };                                                                           \
  memoir::Type *const NAME::_type = MEMOIR_FUNC(define_struct_type)(           \
      #NAME,                                                                   \
      MEMOIR_NARGS(FIELDS),                                                    \
      MEMOIR_apply_delim(TO_TYPE_PREPEND, COMMA_DELIM, FIELDS));               \
  template <>                                                                  \
  const type_ref memoir::memoir_type<NAME> = NAME::_type;

#define AUTO_STRUCT(NAME, FIELDS...)                                           \
  TO_CPP_STRUCT(NAME, FIELDS)                                                  \
  TO_MEMOIR_STRUCT(NAME, FIELDS)

#endif // MEMOIR_CPP_OBJECT_HH
