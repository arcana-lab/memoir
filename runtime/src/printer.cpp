#include "objects.h"
#include <iostream>

using namespace objectir;

std::string IntegerField::toString() {
  return "integer";
}

std::string FloatField::toString() {
  return "float";
}

std::string DoubleField::toString() {
  return "double";
}

std::string ObjectField::toString() {
  return "object";
}

std::string PointerField::toString() {
  return "pointer";
}

std::string Object::toString() {
  std::string str = "(Object: \n";
  for (auto field : this->fields) {
    str += "  (Field: ";
    str += "    ";
    str += field->toString();
    str += "  )\n";
  }
  str += ")\n";
  return str;
}

std::string Array::toString() {
  std::string str = "(Array: \n";
  str += "  (type: ";
  str += type->toString();
  str += ")\n";
  str += "  (length: ";
  str += length;
  str += "))\n";
  return str;
}

std::string ObjectType::toString() {
  std::string str = "(Object: \n";
  for (auto field : this->fields) {
    str += "  (Field: \n";
    str += "    " + field->toString();
    str += "  )\n";
  }
  str += ")\n";
  return str;
}

std::string ArrayType::toString() {
  return "(Type: array)";
}

std::string UnionType::toString() {
  return "(Type: union)";
}

std::string IntegerType::toString() {
  return "(Type: integer)";
}

std::string FloatType::toString() {
  return "(Type: float)";
}

std::string DoubleType::toString() {
  return "(Type: double)";
}

std::string PointerType::toString() {
  return "(Type: (pointer "
         + this->containedType->toString() + ")";
}

std::string StubType::toString() {
  return "(Type: stub)";
}
