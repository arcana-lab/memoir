#include <iostream>
#include <string>

#include "internal.h"
#include "objects.h"
#include "types.h"

namespace memoir {
namespace detail {

std::string Struct::to_string() {
  std::string str = "(Struct: \n";
  for (auto field : this->fields) {
    str += "  (Field: ";
    str += "    ";
    // TODO: decode the element and print it.
    str += std::to_string(field);
    str += "  )\n";
  }
  str += ")\n";
  return str;
}

std::string AssocArray::to_string() {
  return "(AssocArray)";
}

std::string SequenceAlloc::to_string() {
  return "(SequenceAlloc)";
}

std::string SequenceView::to_string() {
  return "(SequenceView)";
}

} // namespace detail

/*
 * Types
 */
std::string StructType::to_string() {
  std::string str = "(Struct: \n";
  for (auto field : this->fields) {
    str += "  (Field: \n";
    str += "    " + field->to_string();
    str += "  )\n";
  }
  str += ")\n";
  return str;
}

std::string AssocArrayType::to_string() {
  return "(Type: assoc array \n  (Key: " + this->key_type->to_string()
         + "  )\n"
           "   (Value: "
         + this->value_type->to_string() + "))";
}

std::string SequenceType::to_string() {
  return "(Type: sequence (Element: " + this->element_type->to_string() + "))";
}

std::string IntegerType::to_string() {
  return "(Type: integer)";
}

std::string FloatType::to_string() {
  return "(Type: float)";
}

std::string DoubleType::to_string() {
  return "(Type: double)";
}

std::string PointerType::to_string() {
  return "(Type: pointer)";
}

std::string VoidType::to_string() {
  return "(Type: void)";
}

std::string ReferenceType::to_string() {
  return "(Type: (ref " + this->referenced_type->to_string() + ")";
}

} // namespace memoir
