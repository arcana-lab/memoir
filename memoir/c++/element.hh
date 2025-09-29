#ifndef MEMOIR_CPP_ELEMENT_HH
#define MEMOIR_CPP_ELEMENT_HH

#include <functional>

#include "memoir.h"

#include "memoir++/object.hh"

namespace memoir {

template <typename Ret, typename... Args>
class Functor {
public:
  always_inline Ret operator()(Args... args) const;
};

template <typename T, typename... Indices>
class Element {
protected:
  Functor<T> read;
  Functor<void, const T &> write;

public:
  // Construct an element.
  // always_inline Element(memoir::Collection *const object,
  //                       std::tuple<Indices...> indices)
  //   : read{ [object, indices] },
  //     write{ indices } {}

  always_inline Element(memoir::Collection *const object, Indices... indices)
    : read{ [object, indices...]() {
        if constexpr (false) {
          // Stub.
        }
#define HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)                            \
  else if constexpr (std::is_same_v<T, C_TYPE>) {                              \
    return MEMOIR_FUNC(read_##TYPE_NAME)(object, indices...);                  \
  }
#define HANDLE_INTEGER_TYPE(TYPE_NAME, C_TYPE, BW, IS_SIGNED)                  \
  else if constexpr (std::is_same_v<T, C_TYPE>) {                              \
    return MEMOIR_FUNC(read_##TYPE_NAME)(object, indices...);                  \
  }
#include <types.def>
      } },
      write{ [object, indices...](const T &val) {
        if constexpr (false) {
          // Stub.
        }
#define HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, _)                            \
  else if constexpr (std::is_same_v<T, C_TYPE>) {                              \
    MEMOIR_FUNC(write_##TYPE_NAME)(val, object, indices...);                   \
  }
#define HANDLE_INTEGER_TYPE(TYPE_NAME, C_TYPE, BW, IS_SIGNED)                  \
  else if constexpr (std::is_same_v<T, C_TYPE>) {                              \
    MEMOIR_FUNC(write_##TYPE_NAME)(val, object, indices...);                   \
  }
#include <types.def>
      } } {
  }

  // Get a nested element from this.
  // template <typename Index>
  // always_inline Element<NestedType<T>, Indices..., Index> operator[](
  //     const Index &index) {
  //   return Element<NestedType<T>, Indices..., Index>(
  //       object,
  //       std::tuple_cat(this->indices, std::make_tuple(index)));
  // }

  // template <typename Index>
  // always_inline const Element<NestedType<T>, Indices..., Index> operator[](
  //     const Index &index) const {
  //   return Element<NestedType<T>, Indices..., Index>(
  //       object,
  //       std::tuple_cat(this->indices, std::make_tuple(index)));
  // }

  // Write to the element.
  always_inline T operator=(const T &val) {
    this->write(val);
    return val;
  }

  // Read from the element.
  always_inline operator T() const {
    return this->read();
  } // operator T()
};

} // namespace memoir

#endif // MEMOIR_CPP_ELEMENT_HH
