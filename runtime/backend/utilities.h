#ifndef MEMOIR_BACKEND_UTILITIES_H
#define MEMOIR_BACKEND_UTILITIES_H

#define cname extern "C"
#define alwaysinline __attribute__((always_inline)) inline
#define used __attribute__((used))

// Macro pre-processor tricks from:
// https://github.com/pfultz2/Cloak/wiki/C-Preprocessor-tricks,-tips,-and-idioms#deferred-expression
#define EMPTY()
#define DEFER(id) id EMPTY()
#define EXPAND(...) __VA_ARGS__

#define _CAT(x, y) x##y
#define CAT(x, y) _CAT(x, y)

#define IIF(c) _CAT(IIF_, c)
#define IIF_0(t, ...) __VA_ARGS__
#define IIF_1(t, ...) t

#define COMPLEMENT(b) CAT(COMPLEMENT_, b)
#define COMPLEMENT_0 1
#define COMPLEMENT_1 0

#define BITAND(x) _CAT(BITAND_, x)
#define BITAND_0(y) 0
#define BITAND_1(y) y

#define NOT(x) CHECK(_CAT(NOT_, x))
#define NOT_0 PROBE(~)

#define BOOL(x) COMPL(NOT(x))
#define IF(c) IIF(BOOL(c))

#define EAT(...)
#define EXPAND(...) __VA_ARGS__
#define WHEN(c) IF(c)(EXPAND, EAT)

#define CHECK_N(x, n, ...) n
#define CHECK(...) CHECK_N(__VA_ARGS__, 0, )
#define PROBE(x) x, 1,

#define IS_PAREN(x) CHECK(IS_PAREN_PROBE x)
#define IS_PAREN_PROBE(...) PROBE(~)

#define COMPARE_u64(x) x
#define COMPARE_u32(x) x
#define COMPARE_u16(x) x
#define COMPARE_u8(x) x
#define COMPARE_i64(x) x
#define COMPARE_i32(x) x
#define COMPARE_i16(x) x
#define COMPARE_i8(x) x
#define COMPARE_boolean(x) x

#define COMPARE_(x, y) IS_PAREN(COMPARE_##x(COMPARE_##y)(()))

#define IS_COMPARABLE(x) IS_PAREN(CAT(COMPARE_, x)(()))

#define NOT_EQUAL(x, y)                                                        \
  IIF(BITAND(IS_COMPARABLE(x))(IS_COMPARABLE(y)))                              \
  (COMPARE_, 1 EAT)(x, y)

#define EQUAL(x, y) COMPLEMENT(NOT_EQUAL(x, y))

#include <cstdint>
#include <functional>

template <int N>
struct Bytes : public std::array<uint8_t, N> {};

template <int N>
struct std::hash<Bytes<N>> {
  std::size_t operator()(const Bytes<N> &xs) const {
    constexpr std::size_t prime{ 0x100000001B3 };
    std::size_t result{ 0xcbf29ce484222325 };
    for (auto x : xs) {
      result = (result * prime) ^ x;
    }
    return result;
  }
};

#define DEF_HASH(CLASS)                                                        \
  template <>                                                                  \
  struct std::hash<CLASS> {                                                    \
    std::size_t operator()(const CLASS &xs) const {                            \
      constexpr std::size_t prime{ 0x100000001B3 };                            \
      std::size_t result{ 0xcbf29ce484222325 };                                \
      for (auto x : xs) {                                                      \
        result = (result * prime) ^ x;                                         \
      }                                                                        \
      return result;                                                           \
    }                                                                          \
  }

template <class T>
struct is_nested
  : std::negation<std::integral_constant<bool,
                                         std::is_arithmetic<T>::value
                                             or std::is_pointer<T>::value>> {};

template <class T>
constexpr bool is_nested_v = is_nested<T>::value;

template <class T>
struct as_primitive
  : std::conditional<is_nested_v<T>, std::add_pointer_t<T>, T> {};

template <class T>
using as_primitive_t = typename as_primitive<T>::type;

template <class T>
constexpr auto into_primitive(T &value) noexcept {
  if constexpr (is_nested_v<T>) {
    return &value;
  } else {
    return value;
  }
}

#endif // MEMOIR_BACKEND_UTILITIES_H
