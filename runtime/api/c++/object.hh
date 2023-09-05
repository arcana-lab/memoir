#ifndef MEMOIR_CPP_OBJECT_HH
#define MEMOIR_CPP_OBJECT_HH
#pragma once

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <list>
#include <unordered_set>

#include "refl-cpp/include/refl.hpp"

#include <memoir.h>

namespace memoir {

/*
 * Helper types and functions.
 */
template <typename T>
constexpr auto readable_members = filter(refl::member_list<T>{},
                                         [](auto member) {
                                           return is_readable(member);
                                         });

template <typename T>
using readable_member_list = std::remove_const_t<decltype(readable_members<T>)>;

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

template <typename Arg, typename... Args>
static constexpr decltype(auto) peel(Arg &&arg, Args &&... args) {
  return arg;
}

template <class T, template <class...> class Template>
constexpr bool is_specialization{ false };

template <template <class...> class Template, class... Args>
constexpr bool is_specialization<Template<Args...>, Template>{ true };

/*
 * A C++ wrapper for a memoir type.
 */
template <typename T>
class type {
public:
  using readable_members = readable_member_list<std::remove_cv_t<T>>;
  static_assert(readable_members::size > 0, "Type has no fields!");

  static memoir::Type *create_memoir_type() noexcept {
    std::array<memoir::Type *, readable_members::size> field_types;

    for_each(readable_members{}, [&](auto member, size_t index) {
      constexpr auto i =
          refl::trait::index_of_v<decltype(member), readable_members>;

      using field_type =
          typename refl::trait::get_t<i, readable_members>::value_type;

      if constexpr (std::is_pointer<field_type>::value) {
        using inner_type = typename std::remove_pointer<field_type>::type;
        field_types[i] = MEMOIR_FUNC(ref_type)(type<inner_type>::memoir_type);
      }
#define HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)                            \
  else if constexpr (std::is_same<field_type, C_TYPE>::value) {                \
    field_types[i] = MEMOIR_FUNC(TYPE_NAME##_type)();                          \
  }
#define HANDLE_INTEGER_TYPE(TYPE_NAME, C_TYPE, BW, IS_SIGNED)                  \
  HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)
#include <types.def>
#undef HANDLE_INTEGER_TYPE
#undef HANDLE_PRIMITIVE_TYPE
      else {
        // std::cerr << "Couldn't determine field type" << std::endl;
      }
    });

    return array_to_type(field_types);
  }

  static memoir::Type *memoir_type;

private:
  template <typename Array, std::size_t... I>
  static auto type_impl(const Array &arr,
                        std::index_sequence<I...> idx_seq) noexcept {
    auto type_name = refl::descriptor::get_name(refl::reflect<T>()).c_str();
    return MEMOIR_FUNC(
        define_struct_type)(type_name, idx_seq.size(), arr[I]...);
  }

  template <std::size_t N, typename Indices = std::make_index_sequence<N>>
  static auto array_to_type(const std::array<memoir::Type *, N> &arr) noexcept {
    return type_impl(arr, Indices{});
  }
};

template <typename T>
memoir::Type *type<T>::memoir_type = type<T>::create_memoir_type();

/*
 * C++ wrapper for primitive types
 */
template <typename T>
class primitive_type {
  static memoir::Type *create_memoir_type() noexcept {
    if constexpr (false) {
      // Do nothing.
    }
#define HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)                            \
  else if constexpr (std::is_same<T, C_TYPE>::value) {                         \
    return MEMOIR_FUNC(TYPE_NAME##_type)();                                    \
  }
#define HANDLE_INTEGER_TYPE(TYPE_NAME, C_TYPE, BW, IS_SIGNED)                  \
  HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)
#include <types.def>
#undef HANDLE_INTEGER_TYPE
#undef HANDLE_PRIMITIVE_TYPE
    else {
      return nullptr;
    }
  }

public:
  static memoir::Type *memoir_type;
};

template <typename T>
memoir::Type *primitive_type<T>::memoir_type =
    primitive_type<T>::create_memoir_type();

template <typename T>
class object : public refl::runtime::proxy<object<T>, std::remove_cv_t<T>> {
public:
  using readable_members = readable_member_list<std::remove_cv_t<T>>;
  static_assert(readable_members::size > 0, "Type has no fields!");

  memoir::Struct *target_object;

  /*
   * Initialize memoir object
   */
  constexpr object() noexcept
    : target_object(MEMOIR_FUNC(allocate_struct)(type<T>::memoir_type)) {}

  constexpr object(memoir::Struct *&&init) : target_object(init) {}

  // constexpr object(object &&other)
  //   : target_object(std::copy(other.target_object)) {}

  /*
   * Override the address operator
   */
  // constexpr memoir::Object *operator&() const {
  //   return std::forward<memoir::Object *>(this->target_object);
  // }

  constexpr const object *operator&() const {
    return this;
  }

  constexpr object &operator*() const {
    return *this;
  }

  /*
   * Handle accesses
   */
  template <typename Member, typename Self, typename... Args>
  static constexpr decltype(auto) invoke_impl(Self &&self,
                                              Args &&... args) noexcept {
    constexpr Member member;

    if constexpr (is_field(member)) {
      static_assert(sizeof...(Args) <= 1,
                    "Invalid number of arguments for get/set");

      constexpr auto i = refl::trait::index_of_v<Member, readable_members>;
      static_assert(i < readable_members::size, "Invalid member");

      using field_type =
          typename refl::trait::get_t<i, readable_members>::value_type;

      if constexpr (sizeof...(Args) == 1) {
        static_assert(is_writable(member));
        if constexpr (std::is_pointer<field_type>::value) {
          if constexpr (refl::trait::is_reflectable<field_type>::value) {
            // if constexpr (std::is_base_of<field_type, object>::value) {
            using inner_type = typename std::remove_pointer<field_type>::type;
            auto &&peeled_arg =
                static_cast<memoir::object<inner_type>>(peel((args)...));
            MEMOIR_FUNC(struct_write_struct_ref)
            (peeled_arg.target_object, self.target_object, i);
            // }
          }
        }
#define HANDLE_TYPE(TYPE_NAME, C_TYPE)                                         \
  else if constexpr (std::is_same<field_type, C_TYPE>::value) {                \
    auto &&peeled_arg = static_cast<C_TYPE>(peel((args)...));                  \
    memoir::MEMOIR_FUNC(struct_write_##TYPE_NAME)(                             \
        std::forward<C_TYPE>(peeled_arg),                                      \
        self.target_object,                                                    \
        i);                                                                    \
  }
#include <types.def>
#undef HANDLE_TYPE
        else {
          // Do nothing.
        }

      } else {
        static_assert(is_readable(member));
        if constexpr (std::is_pointer<field_type>::value) {
          if constexpr (refl::trait::is_reflectable_v<field_type>) {
            // if constexpr (std::is_base_of<field_type, object>::value) {
            using inner_type = typename std::remove_pointer_t<field_type>;
            return new object<inner_type>(
                MEMOIR_FUNC(struct_read_struct_ref)(self.target_object, i));
            // }
          }
        }
#define HANDLE_TYPE(TYPE_NAME, C_TYPE)                                         \
  else if constexpr (std::is_same<field_type, C_TYPE>::value) {                \
    return memoir::MEMOIR_FUNC(struct_read_##TYPE_NAME)(self.target_object,    \
                                                        i);                    \
  }
#include <types.def>
#undef HANDLE_TYPE
        else {
          // Do nothing.
        }
      }
    }
  }
}; // namespace memoir

/*
 * Primitive type overrides
 */

template <typename T>
class vector {
  static_assert(is_specialization<remove_all_pointers_t<T>, memoir::object>,
                "Trying to store non memoir object in a memoir collection!");

public:
  T &at(std::size_t idx) {
    return _storage.at(idx);
  }

  T &operator[](std::size_t idx) {
    return _storage[idx];
  }

  const T &operator[](std::size_t idx) const {
    return _storage[idx];
  }

  const T &front() const {
    return _storage.front();
  }

  const T &back() const {
    return _storage.back();
  }

  // T *data() noexcept {
  //   return _storage.data();
  // }

  // const T *data() const noexcept {
  //   return _storage.data();
  // }

private:
  std::vector<T> _storage;
};

template <typename Key>
class set {
  static_assert(is_specialization<remove_all_pointers_t<Key>, memoir::object>,
                "Trying to store non memoir object in a memoir collection!");

  using iterator = typename std::unordered_set<Key>::iterator;

public:
  iterator begin() noexcept {
    return _storage.begin();
  }

  iterator end() noexcept {
    return _storage.end();
  }

  bool empty() const noexcept {
    return _storage.empty();
  }

  std::size_t size() const noexcept {
    return _storage.size();
  }

  void clear() noexcept {
    _storage.clear();
  }

  void erase(iterator pos) {
    _storage.erase(pos);
  }

  void erase(const Key &key) {
    _storage.erase(key);
  }

  void insert(const Key &key) {
    _storage.insert(key);
  }

  std::size_t count(const Key &key) {
    return _storage.count(key);
  }

  iterator find(const Key &key) {
    return _storage.find(key);
  }

  bool contains(const Key &key) {
    return (_storage.find(key) != _storage.end());
  }

private:
  std::unordered_set<Key> _storage;
};

template <typename T>
class list {
  static_assert(is_specialization<remove_all_pointers_t<T>, memoir::object>,
                "Trying to store non memoir object in a memoir collection!");

  using iterator = typename std::list<T>::iterator;

public:
  /*
   * Element access
   */
  T &front() {
    return _storage.front();
  }

  T &back() {
    return _storage.back();
  }

  /*
   * Iterators
   */
  iterator begin() noexcept {
    return _storage.begin();
  }

  iterator end() noexcept {
    return _storage.end();
  }

  /*
   * Capacity
   */
  bool empty() const noexcept {
    return _storage.empty();
  }

  std::size_t size() const noexcept {
    return _storage.size();
  }

  /*
   * Modifiers
   */
  void clear() noexcept {
    _storage.clear();
  }

  void erase(iterator pos) {
    _storage.erase(pos);
  }

  void push_back(const T &key) {
    _storage.push_back(key);
  }

  void pop_back(const T &key) {
    _storage.pop_back(key);
  }

  void push_front(const T &key) {
    _storage.push_front(key);
  }

  void pop_front(const T &key) {
    _storage.pop_front(key);
  }

  iterator insert(iterator pos, const T &key) {
    return _storage.insert(pos, key);
  }

  /*
   * Operations
   */
  // void merge(list &other) {
  //   _storage.merge(other);
  // }

  // void splice(iterator pos, list &other) {
  //   _storage.splice(pos, other);
  // }

  void remove(const T &value) {
    _storage.remove(value);
  }

  void reverse() noexcept {
    _storage.reverse();
  }

  // void unique() {
  //   _storage.unique();
  // }

  void sort() noexcept {
    _storage.sort();
  }

private:
  std::list<T> _storage;
};

/*
 * These macros let you set up a memoir struct, with both the C++ struct and the
 * refl-cpp macro code.
 *
 * Example:
 * AUTO_STRUCT(
 *   MyStruct,
 *   FIELD(uint64_t, a),
 *   FIELD(double, b),
 *   FIELD(MyStruct *, ptr)
 * )
 */
#define AUTO_STRUCT(NAME, FIELDS...)                                           \
  TO_CPP_STRUCT(NAME, FIELDS)                                                  \
  TO_REFL_STRUCT(NAME, FIELDS)                                                 \
  USING_STRUCT(NAME)

#define TO_CPP_FIELD(FIELD_TYPE, FIELD_NAME) FIELD_TYPE FIELD_NAME

#define TO_CPP_DELIM() ;
#define TO_CPP_PREPEND_(F) TO_CPP_##F
#define TO_CPP_PREPEND(F) TO_CPP_PREPEND_(F)

#define TO_CPP_STRUCT(NAME, FIELDS...)                                         \
  namespace memoir::user {                                                     \
  struct NAME {                                                                \
    MEMOIR_apply_delim(TO_CPP_PREPEND, TO_CPP_DELIM, FIELDS);                  \
  };                                                                           \
  }

#define TO_REFL_FIELD(FIELD_TYPE, FIELD_NAME) REFL_FIELD(FIELD_NAME)

#define TO_REFL_PREPEND_(F) TO_REFL_##F
#define TO_REFL_PREPEND(F) TO_REFL_PREPEND_(F)

#define TO_REFL_STRUCT(NAME, FIELDS...)                                        \
  REFL_TYPE(memoir::user::NAME)                                                \
  MEMOIR_apply(TO_REFL_PREPEND, FIELDS) REFL_END

#define USING_STRUCT(NAME) using NAME = memoir::object<memoir::user::NAME>;

} // namespace memoir

#endif // MEMOIR_CPP_OBJECT_HH
