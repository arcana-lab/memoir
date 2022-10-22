#include "objects.h"
#include <iostream>

namespace memoir {

std::string IntegerField::toString() {
  return "integer";
}

std::string FloatField::toString() {
  return "float";
}

std::string DoubleField::toString() {
  return "double";
}

std::string StructField::toString() {
  return "struct";
}

std::string TensorField::toString() {
  return "tensor";
}

std::string ReferenceField::toString() {
  return "reference";
}

std::string Struct::toString() {
  std::string str = "(Struct: \n";
  for (auto field : this->fields) {
    str += "  (Field: ";
    str += "    ";
    str += field->toString();
    str += "  )\n";
  }
  str += ")\n";
  return str;
}

std::string Tensor::toString() {
  std::string str = "(Tensor: \n";
  str += "  (type: ";
  str += type->toString();
  str += ")\n";
  str += "  (length: ";
  // str += length;
  str += "))\n";
  return str;
}

std::string StructType::toString() {
  std::string str = "(Object: \n";
  for (auto field : this->fields) {
    str += "  (Field: \n";
    str += "    " + field->toString();
    str += "  )\n";
  }
  str += ")\n";
  return str;
}

std::string TensorType::toString() {
  return "(Type: tensor)";
}

std::string StaticTensorType::toString() {
  return "(Type: staticTensor)";
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

std::string ReferenceType::toString() {
  return "(Type: (pointer " + this->referenced_type->toString() + ")";
}

} // namespace memoir
