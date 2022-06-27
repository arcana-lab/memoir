/*
 * Object representation recognizable by LLVM IR
 * This file contains methods to access object-ir
 * objects, fields and types for the object-ir library.
 *
 * Author(s): Tommy McMichen
 * Created: Mar 17, 2022
 */

#include <iostream>

#include "object_ir.h"

namespace memoir {

extern "C" {

/*
 * Object accesses
 */
__RUNTIME_ATTR
Field *getStructField(Object *object, uint64_t field_index) {
  auto type = object->getType();
  if (type->getCode() != TypeCode::StructTy) {
    std::cerr << "getStructField: object is of non-struct type\n";
  }

  auto strct = (Struct *)object;
  return strct->readField(field_index);
}

/*
 * Tensor accesses
 *   getTensorElement(
 *     object,
 *     number of dimensions,
 *     <index of dimension, ...>)
 */
__RUNTIME_ATTR
Object *getTensorElement(Object *object, uint64_t num_dimensions, ...) {
  auto type = object->getType();
  if (type->getCode() != TypeCode::TensorTy) {
    std::cerr << "getTensorElement: object is of non-tensor type\n";
  }

  va_list args;

  va_start(args, num_dimensions);

  auto tensor = (Tensor *)object;
  auto current = object;
  for (int i = 0; i < num_dimensions; i++) {
    auto dimension_index = va_arg(args, uint64_t);
    current = tensor->getElement(dimension_index);
    if (current->getType()->getCode() == TypeCode::TensorTy) {
      tensor = (Tensor *)object;
    } else {
      return current;
    }
  }

  va_end(args);

  return tensor;
}

/*
 * Type checking
 */
__RUNTIME_ATTR
bool assertType(Type *type, Object *object) {
  if (object == nullptr) {
    return isObjectType(type);
  }

  if (!type->equals(object->getType())) {
    std::cerr << "assertType: Object is not the correct type\n";
    exit(1);
  }
  return true;
}

__RUNTIME_ATTR
bool setReturnType(Type *type) {
  return true;
}

/*
 * Field accesses
 */
// Unsigned integer access
__RUNTIME_ATTR
void writeUInt64(Field *field, uint64_t value) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    intField->value = value;
  } else {
    std::cerr << "ERROR: Attempt to read UInt64 from non-integer field\n";
    exit(1);
  }
}

__RUNTIME_ATTR
void writeUInt32(Field *field, uint32_t value) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    intField->value = (uint64_t)value;
  } else {
    std::cerr << "ERROR: Attempt to read UInt32 from non-integer field\n";
    exit(1);
  }
}

__RUNTIME_ATTR
void writeUInt16(Field *field, uint16_t value) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    intField->value = (uint64_t)value;
  } else {
    std::cerr << "ERROR: Attempt to read UInt16 from non-integer field\n";
    exit(1);
  }
}

__RUNTIME_ATTR
void writeUInt8(Field *field, uint8_t value) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    intField->value = (uint64_t)value;
  } else {
    std::cerr << "ERROR: Attempt to read UInt8 from non-integer field\n";
    exit(1);
  }
}

// Signed integer access
__RUNTIME_ATTR
void writeInt64(Field *field, int64_t value) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    intField->value = (uint64_t)value;
  } else {
    std::cerr << "ERROR: Attempt to read Int64 from non-integer field\n";
    exit(1);
  }
}

__RUNTIME_ATTR
void writeInt32(Field *field, int32_t value) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    intField->value = (uint64_t)value;
  } else {
    std::cerr << "ERROR: Attempt to read Int32 from non-integer field\n";
    exit(1);
  }
}

__RUNTIME_ATTR
void writeInt16(Field *field, int16_t value) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    intField->value = (uint64_t)value;
  } else {
    std::cerr << "ERROR: Attempt to read Int16 from non-integer field\n";
    exit(1);
  }
}

__RUNTIME_ATTR
void writeInt8(Field *field, int8_t value) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    intField->value = (uint64_t)value;
  } else {
    std::cerr << "ERROR: Attempt to read Int8 from non-integer field\n";
    exit(1);
  }
}

// Boolean access
__RUNTIME_ATTR
void writeBoolean(Field *field, bool value) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    intField->value = (uint64_t)value;
  } else {
    std::cerr << "ERROR: Attempt to read Int8 from non-integer field\n";
    exit(1);
  }
}

// Floating point access
__RUNTIME_ATTR
void writeFloat(Field *field, float value) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    FloatField *floatField = (FloatField *)field;
    floatField->value = value;
  } else {
    std::cerr << "ERROR: Attempt to read float from non-float field\n";
    exit(1);
  }
}

__RUNTIME_ATTR
void writeDouble(Field *field, double value) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    DoubleField *doubleField = (DoubleField *)field;
    doubleField->value = value;
  } else {
    std::cerr << "ERROR: Attempt to read double from non-double field\n";
    exit(1);
  }
}

// Pointer access
__RUNTIME_ATTR
void writeObject(Field *field, Object *object) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::StructTy) {
    ObjectField *objField = (ObjectField *)field;
    objField->value = object;
  } else {
    std::cerr << "ERROR: Attempt to read UInt64 from non-integer field\n";
    exit(1);
  }
}

// Integer access
__RUNTIME_ATTR
uint64_t readUInt64(Field *field) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    return intField->value;
  } else {
    std::cerr << "ERROR: Attempt to read UInt64 from non-integer field\n";
    exit(1);
  }
}

__RUNTIME_ATTR
uint32_t readUInt32(Field *field) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    return (uint32_t)intField->value;
  } else {
    std::cerr << "ERROR: Attempt to read UInt32 from non-integer field\n";
    exit(1);
  }
}

__RUNTIME_ATTR
uint16_t readUInt16(Field *field) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    return (uint16_t)intField->value;
  } else {
    std::cerr << "ERROR: Attempt to read UInt16 from non-integer field\n";
    exit(1);
  }
}

__RUNTIME_ATTR
uint8_t readUInt8(Field *field) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    return (uint8_t)intField->value;
  } else {
    std::cerr << "ERROR: Attempt to read UInt8 from non-integer field\n";
    exit(1);
  }
}

__RUNTIME_ATTR
int64_t readInt64(Field *field) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    return intField->value;
  } else {
    std::cerr << "ERROR: Attempt to read Int64 from non-integer field\n";
    exit(1);
  }
}

__RUNTIME_ATTR
int32_t readInt32(Field *field) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    return (int32_t)intField->value;
  } else {
    std::cerr << "ERROR: Attempt to read Int32 from non-integer field\n";
    exit(1);
  }
}

__RUNTIME_ATTR
int16_t readInt16(Field *field) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    return (int16_t)intField->value;
  } else {
    std::cerr << "ERROR: Attempt to read Int16 from non-integer field\n";
    exit(1);
  }
}

__RUNTIME_ATTR
int8_t readInt8(Field *field) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    return (int8_t)intField->value;
  } else {
    std::cerr << "ERROR: Attempt to read Int8 from non-integer field\n";
    exit(1);
  }
}

// Floating point access
__RUNTIME_ATTR
float readFloat(Field *field) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::FloatTy) {
    auto float_field = (FloatField *)field;
    return float_field->value;
  } else {
    std::cerr << "ERROR: Attempt to read float from non-float field\n";
    exit(1);
  }
}

// Double precistion floating point access
__RUNTIME_ATTR
double readDouble(Field *field) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::DoubleTy) {
    auto double_field = (DoubleField *)field;
    return double_field->value;
  } else {
    std::cerr << "ERROR: Attempt to read doube from non-double field\n";
    exit(1);
  }
}

// Object access
__RUNTIME_ATTR
Object *readObject(Field *field) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::StructTy) {
    auto object_field = (ObjectField *)field;
    return object_field->value;
  } else {
    std::cerr << "ERROR: Attempt to read UInt64 from non-integer field\n";
    exit(1);
  }
}

// Reference access
__RUNTIME_ATTR
Object *readReference(Field *field) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::ReferenceTy) {
    auto reference_field = (ReferenceField *)field;
    return reference_field->readValue();
  } else {
    std::cerr << "ERROR: Trying to read reference from non-reference field\n";
    exit(1);
  }
}

__RUNTIME_ATTR
void writeReference(Field *field, Object *obj) {
  TypeCode field_type = field->getType()->getCode();
  if (field_type == TypeCode::ReferenceTy) {
    auto reference_field = (ReferenceField *)field;
    reference_field->writeValue(obj);
  } else {
    std::cerr << "ERROR: Trying to write reference to non-reference field\n";
    exit(1);
  }
}

} // extern "C"
} // namespace memoir
