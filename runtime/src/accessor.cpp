/*
 * Object representation recognizable by LLVM IR
 * This file contains methods to access object-ir
 * objects, fields and types for the object-ir library.
 *
 * Author(s): Tommy McMichen
 * Created: Mar 17, 2022
 */

#include <iostream>

#include "memoir.h"

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
 *     <index of dimension, ...>)
 */
__RUNTIME_ATTR
Field *getTensorElement(Object *object, ...) {
  auto type = object->getType();
  if (type->getCode() != TypeCode::TensorTy) {
    std::cerr << "getTensorElement: object is of non-tensor type\n";
    exit(1);
  }

  auto tensor = (Tensor *)object;
  auto num_dimensions = tensor->length_of_dimensions.size();

  va_list args;

  va_start(args, object);

  std::vector<uint64_t> indices;
  for (int i = 0; i < num_dimensions; i++) {
    auto dimension_index = va_arg(args, uint64_t);
    auto dimension_length = tensor->length_of_dimensions.at(i);

    if (dimension_index >= dimension_length) {
      std::cerr << "getTensorElement: dimension out range\n";
      exit(1);
    }

    indices.push_back(dimension_index);
  }

  va_end(args);

  return tensor->getElement(indices);
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
    std::cerr << "ERROR: Attempt to write UInt64 from non-integer field\n";
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
    std::cerr << "ERROR: Attempt to write UInt32 from non-integer field\n";
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
    std::cerr << "ERROR: Attempt to write UInt16 from non-integer field\n";
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
    std::cerr << "ERROR: Attempt to write UInt8 from non-integer field\n";
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
    std::cerr << "ERROR: Attempt to write Int64 from non-integer field\n";
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
    std::cerr << "ERROR: Attempt to write Int32 from non-integer field\n";
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
    std::cerr << "ERROR: Attempt to write Int16 from non-integer field\n";
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
    std::cerr << "ERROR: Attempt to write Int8 from non-integer field\n";
    exit(1);
  }
}

// General integer access
__RUNTIME_ATTR
void writeInteger(Field *field, uint64_t value) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    intField->value = (uint64_t)value;
  } else {
    std::cerr << "ERROR: Attempt to write integer from non-integer field\n";
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
    std::cerr << "ERROR: Attempt to write Int8 from non-integer field\n";
    exit(1);
  }
}

// Floating point access
__RUNTIME_ATTR
void writeFloat(Field *field, float value) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::FloatTy) {
    FloatField *floatField = (FloatField *)field;
    floatField->value = value;
  } else {
    std::cerr << "ERROR: Attempt to write float from non-float field\n";
    exit(1);
  }
}

__RUNTIME_ATTR
void writeDouble(Field *field, double value) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::DoubleTy) {
    DoubleField *doubleField = (DoubleField *)field;
    doubleField->value = value;
  } else {
    std::cerr << "ERROR: Attempt to write double from non-double field\n";
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

// General integer access
__RUNTIME_ATTR
uint64_t readInteger(Field *field) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    return intField->value;
  } else {
    std::cerr << "ERROR: Attempt to read integer from non-integer field\n";
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
Struct *readStruct(Field *field) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::StructTy) {
    auto struct_field = (StructField *)field;
    return struct_field->value;
  } else {
    std::cerr << "ERROR: Attempt to read struct from non-struct field\n";
    exit(1);
  }
}

__RUNTIME_ATTR
Tensor *readTensor(Field *field) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::TensorTy) {
    auto tensor_field = (TensorField *)field;
    return tensor_field->value;
  } else {
    std::cerr << "ERROR: Attempt to read tensor from non-tensor field\n";
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
