#include "objects.hpp"

using namespace objectir;

Field::Field(Type *type)
  : type(type) {}

IntegerField::IntegerField(uint64_t init, uint64_t bitwidth, bool isSigned)
  : Field(buildIntegerType(bitwidth, isSigned))
  , value(init) {}

IntegerField::IntegerField(uint8_t init)
  : Field(buildUInt8Type())
  , value(init) {}

IntegerField::IntegerField(uint16_t init)
  : Field(buildUInt16Type())
  , value(init) {}

IntegerField::IntegerField(uint32_t init)
  : Field(buildUInt32Type())
  , value(init) {}

IntegerField::IntegerField(uint64_t init)
  : Field(buildUInt64Type())
  , value(init) {}

IntegerField::IntegerField(int8_t init)
  : Field(buildInt8Type())
  , value(init) {}

IntegerField::IntegerField(int16_t init)
  : Field(buildInt16Type())
  , value(init) {}

IntegerField::IntegerField(int32_t init)
  : Field(buildInt32Type())
  , value(init) {}

IntegerField::IntegerField(int64_t init)
  : Field(buildInt64Type())
  , value(init) {}

FloatField::FloatField(float init)
  : Field(buildFloatType())
  , value(init) {}

DoubleField::DoubleField(double init)
  : Field(buildDoubleType())
  , value(init) {}

PointerField::PointerField(Object *obj)
  : Field(obj->getType())
  , value(obj) {}

Type *Field::getType() {
  return this->type;
}
