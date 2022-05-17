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

using namespace objectir;

extern "C" {

/*
 * Object accesses
 */
__OBJECTIR_ATTR
Field *getObjectField(Object *object, uint64_t fieldNo) {
  return object->fields.at(fieldNo);
}

/*
 * Array accesses
 */
__OBJECTIR_ATTR
Field *getArrayElement(Array *array, uint64_t index) {
  return array->fields.at(index);
}

/*
 * Union accesses
 */
__OBJECTIR_ATTR
Field *getUnionMember(Union *unionObj, uint64_t index) {
  return unionObj->members.at(index);
}

/*
 * Type checking
 */
__OBJECTIR_ATTR
bool assertType(Type *type, Object *object) {
  if (!type->equals(object->getType())) {
    std::cerr
        << "assertType: Object is not the correct type\n";
    exit(1);
  }
  return true;
}

__OBJECTIR_ATTR
bool assertFieldType(Type *type, Field *field) {
  if (!type->equals(field->getType())) {
    std::cerr
        << "assertFieldType: Field is not the correct type\n";
    exit(1);
  }
  return true;
}

__OBJECTIR_ATTR
bool setReturnType(Type *type) {
  return true;
}

/*
 * Field accesses
 */
// Unsigned integer access
__OBJECTIR_ATTR
void writeUInt64(Field *field, uint64_t value) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    intField->value = value;
  } else {
    std::cerr
        << "ERROR: Attempt to read UInt64 from non-integer field\n";
    exit(1);
  }
}

__OBJECTIR_ATTR
void writeUInt32(Field *field, uint32_t value) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    intField->value = (uint64_t)value;
  } else {
    std::cerr
        << "ERROR: Attempt to read UInt32 from non-integer field\n";
    exit(1);
  }
}

__OBJECTIR_ATTR
void writeUInt16(Field *field, uint16_t value) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    intField->value = (uint64_t)value;
  } else {
    std::cerr
        << "ERROR: Attempt to read UInt16 from non-integer field\n";
    exit(1);
  }
}

__OBJECTIR_ATTR
void writeUInt8(Field *field, uint8_t value) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    intField->value = (uint64_t)value;
  } else {
    std::cerr
        << "ERROR: Attempt to read UInt8 from non-integer field\n";
    exit(1);
  }
}

// Signed integer access
__OBJECTIR_ATTR
void writeInt64(Field *field, int64_t value) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    intField->value = (uint64_t)value;
  } else {
    std::cerr
        << "ERROR: Attempt to read Int64 from non-integer field\n";
    exit(1);
  }
}

__OBJECTIR_ATTR
void writeInt32(Field *field, int32_t value) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    intField->value = (uint64_t)value;
  } else {
    std::cerr
        << "ERROR: Attempt to read Int32 from non-integer field\n";
    exit(1);
  }
}

__OBJECTIR_ATTR
void writeInt16(Field *field, int16_t value) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    intField->value = (uint64_t)value;
  } else {
    std::cerr
        << "ERROR: Attempt to read Int16 from non-integer field\n";
    exit(1);
  }
}

__OBJECTIR_ATTR
void writeInt8(Field *field, int8_t value) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    intField->value = (uint64_t)value;
  } else {
    std::cerr
        << "ERROR: Attempt to read Int8 from non-integer field\n";
    exit(1);
  }
}

// Boolean access
__OBJECTIR_ATTR
void writeBoolean(Field *field, bool value) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    intField->value = (uint64_t)value;
  } else {
    std::cerr
        << "ERROR: Attempt to read Int8 from non-integer field\n";
    exit(1);
  }
}

// Floating point access
__OBJECTIR_ATTR
void writeFloat(Field *field, float value) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    FloatField *floatField = (FloatField *)field;
    floatField->value = value;
  } else {
    std::cerr
        << "ERROR: Attempt to read float from non-float field\n";
    exit(1);
  }
}

__OBJECTIR_ATTR
void writeDouble(Field *field, double value) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    DoubleField *doubleField = (DoubleField *)field;
    doubleField->value = value;
  } else {
    std::cerr
        << "ERROR: Attempt to read double from non-double field\n";
    exit(1);
  }
}

// Pointer access
__OBJECTIR_ATTR
void writeObject(Field *field, Object *object) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::ObjectTy) {
    ObjectField *objField = (ObjectField *)field;
    objField->value = object;
  } else {
    std::cerr
        << "ERROR: Attempt to read UInt64 from non-integer field\n";
    exit(1);
  }
}

// Integer access
__OBJECTIR_ATTR
uint64_t readUInt64(Field *field) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    return intField->value;
  } else {
    std::cerr
        << "ERROR: Attempt to read UInt64 from non-integer field\n";
    exit(1);
  }
}

__OBJECTIR_ATTR
uint32_t readUInt32(Field *field) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    return (uint32_t)intField->value;
  } else {
    std::cerr
        << "ERROR: Attempt to read UInt32 from non-integer field\n";
    exit(1);
  }
}

__OBJECTIR_ATTR
uint16_t readUInt16(Field *field) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    return (uint16_t)intField->value;
  } else {
    std::cerr
        << "ERROR: Attempt to read UInt16 from non-integer field\n";
    exit(1);
  }
}

__OBJECTIR_ATTR
uint8_t readUInt8(Field *field) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    return (uint8_t)intField->value;
  } else {
    std::cerr
        << "ERROR: Attempt to read UInt8 from non-integer field\n";
    exit(1);
  }
}

__OBJECTIR_ATTR
int64_t readInt64(Field *field) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    return intField->value;
  } else {
    std::cerr
        << "ERROR: Attempt to read Int64 from non-integer field\n";
    exit(1);
  }
}

__OBJECTIR_ATTR
int32_t readInt32(Field *field) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    return (int32_t)intField->value;
  } else {
    std::cerr
        << "ERROR: Attempt to read Int32 from non-integer field\n";
    exit(1);
  }
}

__OBJECTIR_ATTR
int16_t readInt16(Field *field) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    return (int16_t)intField->value;
  } else {
    std::cerr
        << "ERROR: Attempt to read Int16 from non-integer field\n";
    exit(1);
  }
}

__OBJECTIR_ATTR
int8_t readInt8(Field *field) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::IntegerTy) {
    IntegerField *intField = (IntegerField *)field;
    return (int8_t)intField->value;
  } else {
    std::cerr
        << "ERROR: Attempt to read Int8 from non-integer field\n";
    exit(1);
  }
}

// Floating point access
__OBJECTIR_ATTR
float readFloat(Field *field) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::FloatTy) {
    FloatField *intField = (FloatField *)field;
    return intField->value;
  } else {
    std::cerr
        << "ERROR: Attempt to read UInt64 from non-integer field\n";
    exit(1);
  }
}

__OBJECTIR_ATTR
double readDouble(Field *field) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::DoubleTy) {
    DoubleField *intField = (DoubleField *)field;
    return intField->value;
  } else {
    std::cerr
        << "ERROR: Attempt to read UInt64 from non-integer field\n";
    exit(1);
  }
}

// Object access
__OBJECTIR_ATTR
Object *readObject(Field *field) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::ObjectTy) {
    ObjectField *objField = (ObjectField *)field;
    return objField->value;
  } else {
    std::cerr
        << "ERROR: Attempt to read UInt64 from non-integer field\n";
    exit(1);
  }
}

__OBJECTIR_ATTR
Object *readPointer(Field *field) {
  TypeCode type = field->getType()->getCode();
  if (type == TypeCode::PointerTy) {
    PointerField *ptrField = (PointerField *)field;
    return ptrField->readField();
  } else {
    std::cerr
        << "ERROR: Trying to read pointer from non-pointer field\n";
    exit(1);
  }
}

__OBJECTIR_ATTR
void writePointer(Field *field, Object *obj) {
  TypeCode fieldType = field->getType()->getCode();
  if (fieldType == TypeCode::PointerTy) {
    PointerField *ptrField = (PointerField *)field;
    ptrField->writeField(obj);
  } else {
    std::cerr
        << "ERROR: Trying to write pointer to non-pointer field\n";
    exit(1);
  }
}

} // extern "C"
