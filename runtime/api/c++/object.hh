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
class sequence;

template <typename K, typename T>
class assoc;

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

template <class T, template <class...> class Template>
constexpr bool is_specialization{ false };

template <template <class...> class Template, class... Args>
constexpr bool is_specialization<Template<Args...>, Template>{ true };

// Object interface.
class object {
public:
  object(memoir::Struct *storage) : _storage(storage) {}

  template <typename F, std::size_t field_index>
  struct field {
    field(memoir::Struct *const storage) : _storage(storage) {}

    operator F() const {
      if constexpr (std::is_pointer_v<F>) {
        using inner_type = typename std::remove_pointer_t<F>;
        if constexpr (std::is_base_of_v<memoir::object, inner_type>) {
          return F(memoir::MEMOIR_FUNC(struct_read_struct_ref)(this->_storage,
                                                               field_index));
        } else {
          return memoir::MEMOIR_FUNC(struct_read_ptr)(this->_storage,
                                                      field_index);
        }
      } else if constexpr (std::is_base_of_v<memoir::object, F>) {
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

    F operator=(F f) const {
      if constexpr (std::is_pointer_v<F>) {
        using inner_type = typename std::remove_pointer_t<F>;
        if constexpr (std::is_base_of_v<memoir::object, inner_type>) {
          memoir::MEMOIR_FUNC(struct_write_struct_ref)(f->_storage,
                                                       this->_storage,
                                                       field_index);
          return f;
        } else {
          memoir::MEMOIR_FUNC(struct_read_ptr)(f, this->_storage, field_index);
          return f;
        }
      } else if constexpr (std::is_base_of_v<memoir::object, F>) {
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

    F operator*() const {
      if constexpr (std::is_pointer_v<F>) {
        using inner_type = typename std::remove_pointer_t<F>;
        if constexpr (std::is_base_of_v<memoir::object, inner_type>) {
          return F(memoir::MEMOIR_FUNC(struct_read_struct_ref)(this->_storage,
                                                               field_index));
        } else {
          return memoir::MEMOIR_FUNC(struct_read_ptr)(this->_storage,
                                                      field_index);
        }
      } else if constexpr (std::is_base_of_v<memoir::object, F>) {
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
template <typename T>
inline constexpr memoir::Type *to_memoir_type() {
  if constexpr (std::is_base_of_v<memoir::object, T>) {
    return memoir::MEMOIR_FUNC(struct_type)(typeid(T).name());
  } else if constexpr (is_specialization<T, memoir::sequence>) {
    return memoir::MEMOIR_FUNC(sequence_type)(
        memoir::to_memoir_type<typename T::value_type>());
  } else if constexpr (is_specialization<T, memoir::assoc>) {
    return memoir::MEMOIR_FUNC(assoc_array_type)(
        memoir::to_memoir_type<typename T::key_type>(),
        memoir::to_memoir_type<typename T::value_type>());
  } else if constexpr (std::is_pointer_v<T>) {
    using inner_type = typename std::remove_pointer_t<T>;
    if constexpr (std::is_base_of_v<memoir::object, inner_type>) {
      return memoir::MEMOIR_FUNC(ref_type)(
          memoir::MEMOIR_FUNC(struct_type)(typeid(inner_type).name()));
    } else {
      return memoir::MEMOIR_FUNC(ptr_type)();
    }
  }
#define HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)                            \
  else if constexpr (std::is_same_v<T, C_TYPE>) {                              \
    return memoir::MEMOIR_FUNC(TYPE_NAME##_type)();                            \
  }
#define HANDLE_INTEGER_TYPE(TYPE_NAME, C_TYPE, BW, IS_SIGNED)                  \
  HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)
#include "types.def"
#undef HANDLE_PRIMITIVE_TYPE
#undef HANDLE_INTEGER_TYPE
  throw std::runtime_error("unknown field type");
}

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
#define TO_TYPE_FIELD(FIELD_TYPE, FIELD_NAME)                                  \
  memoir::to_memoir_type<FIELD_TYPE>()

#define TO_TYPE_PREPEND_(F) TO_TYPE_##F
#define TO_TYPE_PREPEND(F) TO_TYPE_PREPEND_(F)

#define TO_MEMOIR_STRUCT(NAME, FIELDS...)                                      \
  class NAME : public memoir::object {                                         \
  public:                                                                      \
    NAME(memoir::Struct *obj)                                                  \
      : object(obj),                                                           \
        MEMOIR_apply_delim(TO_INIT_PREPEND, COMMA_DELIM, FIELDS) {}            \
    NAME() : NAME(MEMOIR_FUNC(allocate_struct)(NAME::_type)) {}                \
    NAME(MEMOIR_apply_delim(TO_ARGS_PREPEND, COMMA_DELIM, FIELDS)) : NAME() {  \
      MEMOIR_apply_delim(TO_FIELD_INIT_PREPEND, SEMICOLON_DELIM, FIELDS);      \
    }                                                                          \
    /* Instantiate field members. */                                           \
    constexpr static fameta::counter<__COUNTER__, 0, 1> field_index;           \
    MEMOIR_apply_delim(TO_MEMOIR_PREPEND, SEMICOLON_DELIM, FIELDS);            \
                                                                               \
    static memoir::Type *const _type;                                          \
  };                                                                           \
  memoir::Type *const NAME::_type = MEMOIR_FUNC(define_struct_type)(           \
      #NAME,                                                                   \
      MEMOIR_NARGS(FIELDS),                                                    \
      MEMOIR_apply_delim(TO_TYPE_PREPEND, COMMA_DELIM, FIELDS));

#define AUTO_STRUCT(NAME, FIELDS...)                                           \
  TO_CPP_STRUCT(NAME, FIELDS)                                                  \
  TO_MEMOIR_STRUCT(NAME, FIELDS)

#endif // MEMOIR_CPP_OBJECT_HH
