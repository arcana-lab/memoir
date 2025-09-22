#include "memoir/ir/Object.hpp"
#include "memoir/ir/ControlFlow.hpp"
#include "memoir/ir/TypeCheck.hpp"

namespace memoir {

llvm::Value &Object::value() const {
  return *this->_value;
}

void Object::value(llvm::Value &new_value) {
  this->_value = &new_value;
}

OffsetsRef Object::offsets() const {
  return this->_offsets;
}

Type &Object::type(Type &type, OffsetsRef offsets) {
  auto *nested_type = &type;

  for (auto offset : offsets) {
    if (auto *tuple = dyn_cast<TupleType>(nested_type))
      nested_type = &tuple->getFieldType(offset);
    else if (auto *collection = dyn_cast<CollectionType>(nested_type))
      nested_type = &collection->getElementType();
    else
      MEMOIR_UNREACHABLE("Malformed object offset, or incorrect type");
  }

  return *nested_type;
}

Type &Object::type(llvm::Value &value, OffsetsRef offsets) {
  auto *type = type_of(value);
  return Object::type(*type, offsets);
}

Type &Object::type() const {
  return Object::type(this->value(), this->offsets());
}

llvm::BasicBlock *Object::block() const {
  return parent<llvm::BasicBlock>(this->value());
}

llvm::Function *Object::function() const {
  return parent<llvm::Function>(this->value());
}

llvm::Module &Object::module() const {
  return MEMOIR_SANITIZE(parent<llvm::Module>(this->value()),
                         "Object has no parent module!");
}

bool operator<(const Object &lhs, const Object &rhs) {
  auto *lvalue = &lhs.value(), *rvalue = &rhs.value();
  if (lvalue != rvalue) {
    return lvalue < rvalue;
  }

  auto lsize = lhs.offsets().size(), rsize = rhs.offsets().size();
  if (lsize != rsize) {
    return lsize < rsize;
  }

  for (size_t i = 0; i < lsize; ++i) {
    auto loffset = lhs.offsets()[i], roffset = rhs.offsets()[i];
    if (loffset != roffset) {
      return loffset < roffset;
    }
  }

  return false;
}

bool operator==(const Object &lhs, const Object &rhs) {
  auto *lvalue = &lhs.value(), *rvalue = &rhs.value();
  if (lvalue != rvalue) {
    return false;
  }

  auto lsize = lhs.offsets().size(), rsize = rhs.offsets().size();
  if (lsize != rsize) {
    return false;
  }

  for (size_t i = 0; i < lsize; ++i) {
    auto loffset = lhs.offsets()[i], roffset = rhs.offsets()[i];
    if (loffset != roffset) {
      return false;
    }
  }

  return true;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const Object &obj) {
  auto &value = obj.value();
  os << "(";
  if (auto *memoir = into<MemOIRInst>(value))
    os << *memoir;
  else
    os << pretty(value);
  os << ")";
  for (auto offset : obj.offsets()) {
    if (offset == unsigned(-1)) {
      os << "[*]";
    } else {
      os << "." << std::to_string(offset);
    }
  }

  return os;
}

} // namespace memoir

using namespace memoir;

std::size_t std::hash<Object>::operator()(const Object &obj) const noexcept {
  std::size_t h1 = std::hash<llvm::Value *>{}(&obj.value());
  std::size_t i = 0;
  for (auto offset : obj.offsets()) {
    std::size_t h2 = std::hash<Offset>{}(offset);
    h1 ^= (h2 << ++i);
  }
  return h1;
}
