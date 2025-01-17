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

#endif // MEMOIR_BACKEND_UTILITIES_H
