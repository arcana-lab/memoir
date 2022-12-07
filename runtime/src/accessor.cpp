/*
 * Object representation recognizable by LLVM IR
 * This file contains methods to access object-ir
 * objects, fields and types for the object-ir library.
 *
 * Author(s): Tommy McMichen
 * Created: Mar 17, 2022
 */

#include <cassert>
#include <iostream>

#include "internal.h"
#include "memoir.h"

namespace memoir {

extern "C" {

/*
 * Type checking
 */
__RUNTIME_ATTR
bool MEMOIR_FUNC(assert_type)(Type *type, Object *object) {
  if (object == nullptr) {
    return isObjectType(type);
  }

  MEMOIR_ASSERT((type->equals(object->get_type())),
                "Object is not the correct type");

  return true;
}

__RUNTIME_ATTR
bool MEMOIR_FUNC(set_return_type)(Type *type) {
  return true;
}

/*
 * Object accesses
 */
// Unsigned integer access
__RUNTIME_ATTR
void MEMOIR_FUNC(write_u64)(uint64_t value, Object *object_to_access, ...) {
  MEMOIR_ACCESS_CHECK(object_to_access);

  va_list args;

  va_start(args, object_to_access);

  auto element = object_to_access->get_element(args);

  va_end(args);

  MEMOIR_INTEGER_CHECK(element, 64, false, "u64");

  auto integer_element = static_cast<IntegerElement *>(element);
  integer_element->write_value((uint64_t)value);
}

__RUNTIME_ATTR
void MEMOIR_FUNC(write_u32)(uint32_t value, Object *object_to_access, ...) {
  MEMOIR_ACCESS_CHECK(object_to_access);

  va_list args;

  va_start(args, object_to_access);

  auto element = object_to_access->get_element(args);

  va_end(args);

  MEMOIR_INTEGER_CHECK(element, 32, false, "u32");

  auto integer_element = static_cast<IntegerElement *>(element);
  integer_element->write_value((uint64_t)value);
}

__RUNTIME_ATTR
void MEMOIR_FUNC(write_u16)(uint16_t value, Object *object_to_access, ...) {
  MEMOIR_ACCESS_CHECK(object_to_access);

  va_list args;

  va_start(args, object_to_access);

  auto element = object_to_access->get_element(args);

  va_end(args);

  MEMOIR_INTEGER_CHECK(element, 16, false, "u16");

  auto integer_element = static_cast<IntegerElement *>(element);
  integer_element->write_value((uint64_t)value);
}

__RUNTIME_ATTR
void MEMOIR_FUNC(write_u8)(uint8_t value, Object *object_to_access, ...) {
  MEMOIR_ACCESS_CHECK(object_to_access);

  va_list args;

  va_start(args, object_to_access);

  auto element = object_to_access->get_element(args);

  va_end(args);

  MEMOIR_INTEGER_CHECK(element, 8, false, "u8");

  auto integer_element = static_cast<IntegerElement *>(element);
  integer_element->write_value((uint64_t)value);
}

// Signed integer access
__RUNTIME_ATTR
void MEMOIR_FUNC(write_i64)(int64_t value, Object *object_to_access, ...) {
  MEMOIR_ACCESS_CHECK(object_to_access);

  va_list args;

  va_start(args, object_to_access);

  auto element = object_to_access->get_element(args);

  va_end(args);

  MEMOIR_INTEGER_CHECK(element, 64, true, "i64");

  auto integer_element = static_cast<IntegerElement *>(element);
  integer_element->write_value((uint64_t)value);
}

__RUNTIME_ATTR
void MEMOIR_FUNC(write_i32)(int32_t value, Object *object_to_access, ...) {
  MEMOIR_ACCESS_CHECK(object_to_access);

  va_list args;

  va_start(args, object_to_access);

  auto element = object_to_access->get_element(args);

  va_end(args);

  MEMOIR_INTEGER_CHECK(element, 32, true, "i32");

  auto integer_element = static_cast<IntegerElement *>(element);
  integer_element->write_value((uint64_t)value);
}

__RUNTIME_ATTR
void MEMOIR_FUNC(write_i16)(int16_t value, Object *object_to_access, ...) {
  MEMOIR_ACCESS_CHECK(object_to_access);

  va_list args;

  va_start(args, object_to_access);

  auto element = object_to_access->get_element(args);

  va_end(args);

  MEMOIR_INTEGER_CHECK(element, 16, true, "i16");

  auto integer_element = static_cast<IntegerElement *>(element);
  integer_element->write_value((uint64_t)value);
}

__RUNTIME_ATTR
void MEMOIR_FUNC(write_i8)(int8_t value, Object *object_to_access, ...) {
  MEMOIR_ACCESS_CHECK(object_to_access);

  va_list args;

  va_start(args, object_to_access);

  auto element = object_to_access->get_element(args);

  va_end(args);

  MEMOIR_INTEGER_CHECK(element, 8, false, "i8");

  auto integer_element = static_cast<IntegerElement *>(element);
  integer_element->write_value((uint64_t)value);
}

// Boolean access
__RUNTIME_ATTR
void MEMOIR_FUNC(write_bool)(bool value, Object *object_to_access, ...) {
  MEMOIR_ACCESS_CHECK(object_to_access);

  va_list args;

  va_start(args, object_to_access);

  auto element = object_to_access->get_element(args);

  va_end(args);

  MEMOIR_INTEGER_CHECK(element, 1, false, "bool");

  auto integer_element = static_cast<IntegerElement *>(element);
  integer_element->write_value((uint64_t)value);
}

// Floating point access
__RUNTIME_ATTR
void MEMOIR_FUNC(write_f32)(float value, Object *object_to_access, ...) {
  MEMOIR_ACCESS_CHECK(object_to_access);

  va_list args;

  va_start(args, object_to_access);

  auto element = object_to_access->get_element(args);

  va_end(args);

  MEMOIR_TYPE_CHECK(element, TypeCode::FloatTy);

  auto float_element = static_cast<FloatElement *>(element);
  float_element->write_value(value);
}

__RUNTIME_ATTR
void MEMOIR_FUNC(write_f64)(double value, Object *object_to_access, ...) {
  MEMOIR_ACCESS_CHECK(object_to_access);

  va_list args;

  va_start(args, object_to_access);

  auto element = object_to_access->get_element(args);

  va_end(args);

  MEMOIR_TYPE_CHECK(element, TypeCode::DoubleTy);

  auto double_element = static_cast<DoubleElement *>(element);
  double_element->write_value(value);
}

// Pointer access
__RUNTIME_ATTR
void MEMOIR_FUNC(write_ptr)(void *value, Object *object_to_access, ...) {
  MEMOIR_ACCESS_CHECK(object_to_access);

  va_list args;

  va_start(args, object_to_access);

  auto element = object_to_access->get_element(args);

  va_end(args);

  MEMOIR_TYPE_CHECK(element, TypeCode::PointerTy);

  auto ptr_element = static_cast<PointerElement *>(element);
  ptr_element->write_value(value);
}

// Reference access
__RUNTIME_ATTR
void MEMOIR_FUNC(write_ref)(Object *value, Object *object_to_access, ...) {
  MEMOIR_ACCESS_CHECK(object_to_access);

  va_list args;

  va_start(args, object_to_access);

  auto element = object_to_access->get_element(args);

  va_end(args);

  MEMOIR_TYPE_CHECK(element, TypeCode::ReferenceTy);

  auto ref_element = static_cast<ReferenceElement *>(element);
  ref_element->write_value(value);
}

/*
 * Primitive read access
 */
// Unsigned integer access
__RUNTIME_ATTR
uint64_t MEMOIR_FUNC(read_u64)(Object *object_to_access, ...) {
  MEMOIR_ACCESS_CHECK(object_to_access);

  va_list args;

  va_start(args, object_to_access);

  auto element = object_to_access->get_element(args);

  va_end(args);

  MEMOIR_INTEGER_CHECK(element, 64, false, "u64");

  auto integer_element = static_cast<IntegerElement *>(element);
  return (uint64_t)(integer_element->read_value());
}

__RUNTIME_ATTR
uint32_t MEMOIR_FUNC(read_u32)(Object *object_to_access, ...) {
  MEMOIR_ACCESS_CHECK(object_to_access);

  va_list args;

  va_start(args, object_to_access);

  auto element = object_to_access->get_element(args);

  va_end(args);

  MEMOIR_INTEGER_CHECK(element, 32, false, "u32");

  auto integer_element = static_cast<IntegerElement *>(element);
  return (uint32_t)(integer_element->read_value());
}

__RUNTIME_ATTR
uint16_t MEMOIR_FUNC(read_u16)(Object *object_to_access, ...) {
  MEMOIR_ACCESS_CHECK(object_to_access);

  va_list args;

  va_start(args, object_to_access);

  auto element = object_to_access->get_element(args);

  va_end(args);

  MEMOIR_INTEGER_CHECK(element, 16, false, "u16");

  auto integer_element = static_cast<IntegerElement *>(element);
  return (uint16_t)(integer_element->read_value());
}

__RUNTIME_ATTR
uint8_t MEMOIR_FUNC(read_u8)(Object *object_to_access, ...) {
  MEMOIR_ACCESS_CHECK(object_to_access);

  va_list args;

  va_start(args, object_to_access);

  auto element = object_to_access->get_element(args);

  va_end(args);

  MEMOIR_INTEGER_CHECK(element, 8, false, "u8");

  auto integer_element = static_cast<IntegerElement *>(element);
  return (uint8_t)(integer_element->read_value());
}

// Signed integer access
__RUNTIME_ATTR
int64_t MEMOIR_FUNC(read_i64)(Object *object_to_access, ...) {
  MEMOIR_ACCESS_CHECK(object_to_access);

  va_list args;

  va_start(args, object_to_access);

  auto element = object_to_access->get_element(args);

  va_end(args);

  MEMOIR_INTEGER_CHECK(element, 64, true, "i64");

  auto integer_element = static_cast<IntegerElement *>(element);
  return (int64_t)(integer_element->read_value());
}

__RUNTIME_ATTR
int32_t MEMOIR_FUNC(read_i32)(Object *object_to_access, ...) {
  MEMOIR_ACCESS_CHECK(object_to_access);

  va_list args;

  va_start(args, object_to_access);

  auto element = object_to_access->get_element(args);

  va_end(args);

  MEMOIR_INTEGER_CHECK(element, 32, true, "i32");

  auto integer_element = static_cast<IntegerElement *>(element);
  return (int32_t)(integer_element->read_value());
}

__RUNTIME_ATTR
int16_t MEMOIR_FUNC(read_i16)(Object *object_to_access, ...) {
  MEMOIR_ACCESS_CHECK(object_to_access);

  va_list args;

  va_start(args, object_to_access);

  auto element = object_to_access->get_element(args);

  va_end(args);

  MEMOIR_INTEGER_CHECK(element, 16, true, "i16");

  auto integer_element = static_cast<IntegerElement *>(element);
  return (int16_t)(integer_element->read_value());
}

__RUNTIME_ATTR
int8_t MEMOIR_FUNC(read_i8)(Object *object_to_access, ...) {
  MEMOIR_ACCESS_CHECK(object_to_access);

  va_list args;

  va_start(args, object_to_access);

  auto element = object_to_access->get_element(args);

  va_end(args);

  MEMOIR_INTEGER_CHECK(element, 8, false, "i8");

  auto integer_element = static_cast<IntegerElement *>(element);
  return (int8_t)(integer_element->read_value());
}

// Boolean access
__RUNTIME_ATTR
bool MEMOIR_FUNC(read_bool)(Object *object_to_access, ...) {
  MEMOIR_ACCESS_CHECK(object_to_access);

  va_list args;

  va_start(args, object_to_access);

  auto element = object_to_access->get_element(args);

  va_end(args);

  MEMOIR_INTEGER_CHECK(element, 1, false, "bool");

  auto integer_element = static_cast<IntegerElement *>(element);
  return (bool)(integer_element->read_value());
}

// Floating point access
__RUNTIME_ATTR
float MEMOIR_FUNC(read_f32)(Object *object_to_access, ...) {
  MEMOIR_ACCESS_CHECK(object_to_access);

  va_list args;

  va_start(args, object_to_access);

  auto element = object_to_access->get_element(args);

  va_end(args);

  MEMOIR_TYPE_CHECK(element, TypeCode::FloatTy);

  auto float_element = static_cast<FloatElement *>(element);
  return (float)float_element->read_value();
}

__RUNTIME_ATTR
double MEMOIR_FUNC(read_f64)(Object *object_to_access, ...) {
  MEMOIR_ACCESS_CHECK(object_to_access);

  va_list args;

  va_start(args, object_to_access);

  auto element = object_to_access->get_element(args);

  va_end(args);

  MEMOIR_TYPE_CHECK(element, TypeCode::DoubleTy);

  auto double_element = static_cast<DoubleElement *>(element);
  return (double)(double_element->read_value());
}

// Pointer access
__RUNTIME_ATTR
void *MEMOIR_FUNC(read_ptr)(Object *object_to_access, ...) {
  MEMOIR_ACCESS_CHECK(object_to_access);

  va_list args;

  va_start(args, object_to_access);

  auto element = object_to_access->get_element(args);

  va_end(args);

  MEMOIR_TYPE_CHECK(element, TypeCode::PointerTy);

  auto ptr_element = static_cast<PointerElement *>(element);
  return (void *)(ptr_element->read_value());
}

// Reference access
__RUNTIME_ATTR
Object *MEMOIR_FUNC(read_ref)(Object *object_to_access, ...) {
  MEMOIR_ACCESS_CHECK(object_to_access);

  va_list args;

  va_start(args, object_to_access);

  auto element = object_to_access->get_element(args);

  va_end(args);

  MEMOIR_TYPE_CHECK(element, TypeCode::ReferenceTy);

  auto ref_element = static_cast<ReferenceElement *>(element);
  return (Object *)(ref_element->read_value());
}

/*
 * Nested Object access
 */
__RUNTIME_ATTR
Object *MEMOIR_FUNC(get_object)(Object *object_to_access, ...) {
  MEMOIR_ACCESS_CHECK(object_to_access);

  va_list args;

  va_start(args, object_to_access);

  auto element = object_to_access->get_element(args);

  va_end(args);

  MEMOIR_OBJECT_CHECK(element);

  auto object_element = static_cast<ObjectElement *>(element);
  return object_element->read_value();
}

} // extern "C"
} // namespace memoir
