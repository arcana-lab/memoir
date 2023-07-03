#include <cstdio>
#include <iostream>

#include "internal.h"
#include "objects.h"

namespace memoir {

/*
 * Element Constructors
 */

Element::Element(Type *type) : Element(type, 0) {
  // Do nothing.
}

Element::Element(Type *type, uint64_t init) : Object(type), value(init) {
  // Do nothing.
}

IntegerElement::IntegerElement(IntegerType *type) : IntegerElement(type, 0) {
  // Do nothing.
}

IntegerElement::IntegerElement(IntegerType *type, uint64_t init)
  : Element(type, (uint64_t)init) {
  // Do nothing.
}

FloatElement::FloatElement(FloatType *type) : FloatElement(type, 0.0) {
  // Do nothing.
}

FloatElement::FloatElement(FloatType *type, float init)
  : Element(type, (uint64_t)init) {
  // Do nothing.
}

DoubleElement::DoubleElement(DoubleType *type) : DoubleElement(type, 0.0) {
  // Do nothing.
}

DoubleElement::DoubleElement(DoubleType *type, double init)
  : Element(type, (uint64_t)init) {
  // Do nothing.
}

ReferenceElement::ReferenceElement(ReferenceType *type)
  : ReferenceElement(type, nullptr) {
  // Do nothing.
}

ReferenceElement::ReferenceElement(ReferenceType *type, Object *init)
  : Element(type, (uint64_t)init) {
  // Do nothing.
}

PointerElement::PointerElement(PointerType *type)
  : PointerElement(type, nullptr) {
  // Do nothing.
}

PointerElement::PointerElement(PointerType *type, void *init)
  : Element(type, (uint64_t)init) {
  // Do nothing.
}

StructElement::StructElement(StructType *type)
  : StructElement(type, new Struct(type)) {
  // Do nothing.
}

StructElement::StructElement(StructType *type, Struct *init)
  : Element(type, (uint64_t)init) {
  this->value = (uint64_t)this->value;
}

StructElement::~StructElement() {
  delete (Struct *)this->value;
}

CollectionElement::CollectionElement(Type *type) : Element(type) {
  // Do nothing.
}

CollectionElement::CollectionElement(Type *type, Collection *init)
  : Element(type, (uint64_t)init) {
  this->value = (uint64_t)init;
}

TensorElement::TensorElement(TensorType *type)
  : TensorElement(type, new Tensor(type)) {
  // Do nothing.
}

TensorElement::TensorElement(TensorType *type, Tensor *init)
  : CollectionElement(type, init) {
  // Do nothing.
}

TensorElement::~TensorElement() {
  delete (Tensor *)this->value;
}

AssocArrayElement::AssocArrayElement(AssocArrayType *type)
  : AssocArrayElement(type, new AssocArray(type)) {
  // Do nothing.
}

AssocArrayElement::AssocArrayElement(AssocArrayType *type, AssocArray *init)
  : CollectionElement(type, init) {
  // Do nothing.
}

AssocArrayElement::~AssocArrayElement() {
  delete (AssocArray *)this->value;
}

SequenceElement::SequenceElement(SequenceType *type)
  : SequenceElement(type, new Sequence(type, 0)) {
  // Do nothing.
}

SequenceElement::SequenceElement(SequenceType *type, Sequence *init)
  : CollectionElement(type, init) {
  // Do nothing.
}

SequenceElement::~SequenceElement() {
  delete (Sequence *)this->value;
}

/*
 * Element Accessors
 */
void IntegerElement::write_value(uint64_t value) {
  this->value = (uint64_t)value;
}

uint64_t IntegerElement::read_value() const {
  return (uint64_t)this->value;
}

void FloatElement::write_value(float value) {
  this->value = (uint64_t)value;
}

float FloatElement::read_value() const {
  return (float)this->value;
}

void DoubleElement::write_value(double value) {
  this->value = (uint64_t)value;
}

double DoubleElement::read_value() const {
  return (double)this->value;
}

void ReferenceElement::write_value(Object *value) {
  this->value = (uint64_t)value;
}

Object *ReferenceElement::read_value() const {
  return (Object *)this->value;
}

void PointerElement::write_value(void *value) {
  this->value = (uint64_t)value;
}

void *PointerElement::read_value() const {
  return (void *)this->value;
}

Struct *StructElement::read_value() const {
  return (Struct *)this->value;
}

Collection *TensorElement::read_value() const {
  return (Collection *)this->value;
}

Collection *AssocArrayElement::read_value() const {
  return (Collection *)this->value;
}

Collection *SequenceElement::read_value() const {
  return (Collection *)this->value;
}

/*
 * Element cloning
 */
Element *IntegerElement::clone(Element *pos) const {
  auto type = static_cast<IntegerType *>(this->get_type());
  if (pos) {
    return new (pos) IntegerElement(type, (uint64_t)this->value);
  } else {
    return new IntegerElement(type, (uint64_t)this->value);
  }
}

Element *FloatElement::clone(Element *pos) const {
  auto type = static_cast<FloatType *>(this->get_type());
  if (pos) {
    return new (pos) FloatElement(type, (float)this->value);
  } else {
    return new FloatElement(type, (float)this->value);
  }
}

Element *DoubleElement::clone(Element *pos) const {
  auto type = static_cast<DoubleType *>(this->get_type());
  if (pos) {
    return new (pos) DoubleElement(type, (double)this->value);
  } else {
    return new DoubleElement(type, (double)this->value);
  }
}

Element *PointerElement::clone(Element *pos) const {
  auto type = static_cast<PointerType *>(this->get_type());
  if (pos) {
    return new (pos) PointerElement(type, (void *)this->value);
  } else {
    return new PointerElement(type, (void *)this->value);
  }
}

Element *ReferenceElement::clone(Element *pos) const {
  auto type = static_cast<ReferenceType *>(this->get_type());
  if (pos) {
    return new (pos) ReferenceElement(type, (Object *)this->value);
  } else {
    return new ReferenceElement(type, (Object *)this->value);
  }
}

Element *StructElement::clone(Element *pos) const {
  auto cloned_struct = new Struct((Struct *)this->value);
  if (pos) {
    return new (pos) StructElement(static_cast<StructType *>(this->get_type()),
                                   cloned_struct);
  } else {
    return new StructElement(static_cast<StructType *>(this->get_type()),
                             cloned_struct);
  }
}

Element *TensorElement::clone(Element *pos) const {
  // FIXME
  // auto type = static_cast<TensorType *>(this->get_type());
  // auto cloned_tensor = new Tensor(type);
  // auto this_tensor = (Tensor *)this->value;
  // for (auto i = 0; i < this_tensor->tensor.size(); i++) {
  //   auto this_element = Element::decode(this_tensor->get_element_type(),
  //                                       this_tensor->tensor[i]);
  //   cloned_tensor->tensor[i] = this_element->clone();
  // }
  // if (pos) {
  //   return new (pos) TensorElement(type, cloned_tensor);
  // } else {
  //   return new TensorElement(type, cloned_tensor);
  // }
  return nullptr;
}

Element *AssocArrayElement::clone(Element *pos) const {
  // FIXME
  // auto type = static_cast<AssocArrayType *>(this->get_type());
  // auto cloned_assoc_array = new AssocArray(type);
  // cloned_assoc_array->assoc_array.clear();
  // auto this_assoc = (AssocArray *)this->value;
  // cloned_assoc_array->assoc_array.insert(this_assoc->assoc_array.begin(),
  //                                        this_assoc->assoc_array.end());
  // if (pos) {
  //   return new (pos) AssocArrayElement(type, cloned_assoc_array);
  // } else {
  //   return new AssocArrayElement(type, cloned_assoc_array);
  // }
  return nullptr;
}

Element *SequenceElement::clone(Element *pos) const {
  // FIXME
  // auto type = static_cast<SequenceType *>(this->get_type());
  // auto cloned_sequence = new Sequence(type, 0);
  // cloned_sequence->_sequence.insert(cloned_sequence->sequence.begin(),
  //                                   this->value->sequence.begin(),
  //                                   this->value->sequence.end());
  // if (pos) {
  //   return new (pos) SequenceElement(type, cloned_sequence);
  // } else {
  //   return new SequenceElement(type, cloned_sequence);
  // }
  return nullptr;
}

/*
 * Element equality
 */
bool IntegerElement::equals(const Object *other) const {
  MEMOIR_TYPE_CHECK(other, TypeCode::IntegerTy);
  auto other_element = static_cast<const IntegerElement *>(other);
  return (this->read_value() == other_element->read_value());
}

bool FloatElement::equals(const Object *other) const {
  MEMOIR_TYPE_CHECK(other, TypeCode::FloatTy);
  auto other_element = static_cast<const FloatElement *>(other);
  return (this->read_value() == other_element->read_value());
}

bool DoubleElement::equals(const Object *other) const {
  MEMOIR_TYPE_CHECK(other, TypeCode::DoubleTy);
  auto other_element = static_cast<const DoubleElement *>(other);
  return (this->read_value() == other_element->read_value());
}

bool PointerElement::equals(const Object *other) const {
  MEMOIR_TYPE_CHECK(other, TypeCode::PointerTy);
  auto other_element = static_cast<const PointerElement *>(other);
  return (this->read_value() == other_element->read_value());
}

bool ReferenceElement::equals(const Object *other) const {
  MEMOIR_TYPE_CHECK(other, TypeCode::ReferenceTy);
  auto other_element = static_cast<const ReferenceElement *>(other);
  return (this->read_value() == other_element->read_value());
}

bool StructElement::equals(const Object *other) const {
  return (this == other);
}

bool CollectionElement::equals(const Object *other) const {
  return (this == other);
}

/*
 * Helper Functions
 */
bool Element::is_element() const {
  return true;
}

/*
 * Element factory method
 */
Element *Element::create(Type *type, size_t num) {
  switch (type->getCode()) {
    case TypeCode::StructTy: {
      auto *struct_type = static_cast<StructType *>(type);
      auto alloc = (StructElement *)malloc(num * sizeof(StructElement));
      for (auto i = 0; i < num; i++) {
        new (&alloc[i]) StructElement(struct_type);
      }
      return alloc;
    }
    case TypeCode::TensorTy: {
      auto tensor_type = (TensorType *)type;
      MEMOIR_ASSERT((tensor_type->is_static_length),
                    "Attempt to create tensor element of non-static length");
      auto &length_of_dimensions = tensor_type->length_of_dimensions;
      auto alloc = (TensorElement *)malloc(num * sizeof(TensorElement));
      for (auto i = 0; i < num; i++) {
        auto tensor = new Tensor(tensor_type, length_of_dimensions);
        new (&alloc[i]) TensorElement(tensor_type, tensor);
      }
      return alloc;
    }
    case TypeCode::IntegerTy: {
      auto integer_type = static_cast<IntegerType *>(type);
      auto alloc = (IntegerElement *)malloc(num * sizeof(IntegerElement));
      for (auto i = 0; i < num; i++) {
        new (&alloc[i]) IntegerElement(integer_type);
      }
      return alloc;
    }
    case TypeCode::FloatTy: {
      auto float_type = static_cast<FloatType *>(type);
      auto alloc = (FloatElement *)malloc(num * sizeof(FloatElement));
      for (auto i = 0; i < num; i++) {
        new (&alloc[i]) FloatElement(float_type);
      }
      return alloc;
    }
    case TypeCode::DoubleTy: {
      auto double_type = static_cast<DoubleType *>(type);
      auto alloc = (DoubleElement *)malloc(num * sizeof(DoubleElement));
      for (auto i = 0; i < num; i++) {
        new (&alloc[i]) DoubleElement(double_type);
      }
      return alloc;
    }
    case TypeCode::PointerTy: {
      auto ptr_type = static_cast<PointerType *>(type);
      auto alloc = (PointerElement *)malloc(num * sizeof(PointerElement));
      for (auto i = 0; i < num; i++) {
        new (&alloc[i]) PointerElement(ptr_type);
      }
      return alloc;
    }
    case TypeCode::ReferenceTy: {
      auto ref_type = static_cast<ReferenceType *>(type);
      auto alloc = (ReferenceElement *)malloc(num * sizeof(ReferenceElement));
      for (auto i = 0; i < num; i++) {
        new (&alloc[i]) ReferenceElement(ref_type);
      }
      return alloc;
    }
    default: {
      MEMOIR_ASSERT(false, "Attempting to create an element of unknown type");
    }
  }
}

std::vector<uint64_t> Element::init(Type *type, size_t num) {
  switch (type->getCode()) {
    case TypeCode::StructTy: {
      auto *struct_type = static_cast<StructType *>(type);
      std::vector<uint64_t> list;
      list.reserve(num);
      for (auto i = 0; i < num; i++) {
        auto strct = new Struct(struct_type);
        list.push_back((uint64_t)strct);
      }
      return list;
    }
    case TypeCode::TensorTy: {
      auto tensor_type = (TensorType *)type;
      MEMOIR_ASSERT((tensor_type->is_static_length),
                    "Attempt to create tensor element of non-static length");
      auto &length_of_dimensions = tensor_type->length_of_dimensions;
      std::vector<uint64_t> list;
      list.reserve(num);
      for (auto i = 0; i < num; i++) {
        auto tensor = new Tensor(tensor_type, length_of_dimensions);
        list.push_back((uint64_t)tensor);
      }
      return list;
    }
    case TypeCode::IntegerTy: {
      auto integer_type = static_cast<IntegerType *>(type);
      std::vector<uint64_t> list;
      list.reserve(num);
      for (auto i = 0; i < num; i++) {
        list.push_back((uint64_t)0);
      }
      return list;
    }
    case TypeCode::FloatTy: {
      auto float_type = static_cast<FloatType *>(type);
      std::vector<uint64_t> list;
      list.reserve(num);
      for (auto i = 0; i < num; i++) {
        list.push_back((uint64_t)0.0f);
      }
      return list;
    }
    case TypeCode::DoubleTy: {
      auto double_type = static_cast<DoubleType *>(type);
      std::vector<uint64_t> list;
      list.reserve(num);
      for (auto i = 0; i < num; i++) {
        list.push_back((uint64_t)0.0);
      }
      return list;
    }
    case TypeCode::PointerTy: {
      auto ptr_type = static_cast<PointerType *>(type);
      std::vector<uint64_t> list;
      list.reserve(num);
      for (auto i = 0; i < num; i++) {
        list.push_back((uint64_t) nullptr);
      }
      return list;
    }
    case TypeCode::ReferenceTy: {
      auto ref_type = static_cast<ReferenceType *>(type);
      std::vector<uint64_t> list;
      list.reserve(num);
      for (auto i = 0; i < num; i++) {
        list.push_back((uint64_t) nullptr);
      }
      return list;
    }
    default: {
      MEMOIR_ASSERT(false, "Attempting to create an element of unknown type");
    }
  }
}

} // namespace memoir
