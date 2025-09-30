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

// Since this is a header, do not emit function bodies.
#define MEMOIR_BODY

// Include the generated memoir API functions.
#include "memoir/op/memoir.def"

#ifdef __cplusplus
} // namespace memoir
#endif

#endif
