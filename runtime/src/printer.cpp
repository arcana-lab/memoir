#include "objects.hpp"

using namespace objectir;

std::string IntegerField::toString() {
  return "Integer Field";
}

std::string FloatField::toString() {
  return "Float Field";
}

std::string DoubleField::toString() {
  return "Double Field";
}

std::string PointerField::toString() {
  return "Pointer Field";
}

std::string Object::toString() {
  return "Object";
}

std::string Array::toString() {
  return "Array";
}
