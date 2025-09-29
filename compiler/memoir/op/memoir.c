#ifndef MEMOIR_OP_MEMOIR_H
#define MEMOIR_OP_MEMOIR_H

#include "stdarg.h"
#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"

#ifdef __cplusplus
namespace memoir {
#endif

// Specify extern "C" for API functions.
#ifdef __cplusplus
#  define __C_ATTR extern "C"
#else
#  define __C_ATTR
#endif

// Emit empty function bodies.
#define MEMOIR_BODY                                                            \
  {}

// Suppress warnings for empty function bodies.
#pragma clang diagnostic ignored "-Wreturn-type"

// Include the generated memoir API functions.
#include "memoir/op/memoir.inc"

#ifdef __cplusplus
} // namespace memoir
#endif

#endif
