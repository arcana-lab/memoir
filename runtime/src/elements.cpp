#include <cstdio>
#include <iostream>

#include "internal.h"
#include "objects.h"

namespace memoir {

/*
 * Element factory method
 */
std::vector<uint64_t> init_elements(Type *type, size_t num) {
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
    case TypeCode::SequenceTy: {
      auto seq_type = (SequenceType *)type;
      std::vector<uint64_t> list;
      list.reserve(num);
      for (auto i = 0; i < num; i++) {
        auto seq = new SequenceAlloc(seq_type, 0);
        list.push_back((uint64_t)seq);
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

uint64_t init_element(Type *type) {
  switch (type->getCode()) {
    case TypeCode::StructTy: {
      auto *struct_type = static_cast<StructType *>(type);
      auto *strct = new Struct(struct_type);
      return (uint64_t)strct;
    }
    case TypeCode::TensorTy: {
      auto tensor_type = (TensorType *)type;
      MEMOIR_ASSERT((tensor_type->is_static_length),
                    "Attempt to create tensor element of non-static length");
      auto &length_of_dimensions = tensor_type->length_of_dimensions;
      auto tensor = new Tensor(tensor_type, length_of_dimensions);
      return (uint64_t)tensor;
    }
    case TypeCode::SequenceTy: {
      auto seq_type = (SequenceType *)type;
      std::vector<uint64_t> list;
      auto seq = new SequenceAlloc(seq_type, 0);
      return (uint64_t)seq;
    }
    case TypeCode::AssocArrayTy: {
      auto assoc_type = (AssocArrayType *)type;
      auto assoc = new AssocArray(assoc_type);
      return (uint64_t)assoc;
    }
    case TypeCode::IntegerTy: {
      auto integer_type = static_cast<IntegerType *>(type);
      return (uint64_t)0;
    }
    case TypeCode::FloatTy: {
      auto float_type = static_cast<FloatType *>(type);
      return ((uint64_t)0.0f);
    }
    case TypeCode::DoubleTy: {
      auto double_type = static_cast<DoubleType *>(type);
      return ((uint64_t)0.0);
    }
    case TypeCode::PointerTy: {
      auto ptr_type = static_cast<PointerType *>(type);
      return ((uint64_t) nullptr);
    }
    case TypeCode::ReferenceTy: {
      auto ref_type = static_cast<ReferenceType *>(type);
      return ((uint64_t) nullptr);
    }
    default: {
      MEMOIR_ASSERT(false, "Attempting to create an element of unknown type");
    }
  }
}

} // namespace memoir
