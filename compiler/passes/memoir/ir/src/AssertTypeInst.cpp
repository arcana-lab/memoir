#include "memoir/ir/Instructions.hpp"

#include "memoir/analysis/CollectionAnalysis.hpp"
#include "memoir/analysis/StructAnalysis.hpp"
#include "memoir/analysis/TypeAnalysis.hpp"

namespace llvm::memoir {

/*
 * AssertStructTypeInst implementation
 */
Type &AssertStructTypeInst::getType() const {
  return *(TypeAnalysis::get().getType(this->getTypeOperand()));
}

llvm::Value &AssertStructTypeInst::getTypeOperand() const {
  return *(this->getTypeOperandAsUse().get());
}

llvm::Use &AssertStructTypeInst::getTypeOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

Struct &AssertStructTypeInst::getStruct() const {
  auto strct = StructAnalysis::analyze(this->getStructOperand());
  MEMOIR_NULL_CHECK(strct, "Could not determine struct to typecheck");
  return *strct;
}

llvm::Value &AssertStructTypeInst::getStructOperand() const {
  return *(this->getStructOperandAsUse().get());
}

llvm::Use &AssertStructTypeInst::getStructOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}

std::string AssertStructTypeInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "AssertStructTypeInst: " + llvm_str;

  return str;
}

/*
 * AssertCollectionTypeInst implementation
 */
Type &AssertCollectionTypeInst::getType() const {
  auto type = TypeAnalysis::analyze(this->getTypeOperand());
  MEMOIR_NULL_CHECK(type, "Could not determine type to assert");
  return *type;
}

llvm::Value &AssertCollectionTypeInst::getTypeOperand() const {
  return *(this->getTypeOperandAsUse().get());
}

llvm::Use &AssertCollectionTypeInst::getTypeOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

Collection &AssertCollectionTypeInst::getCollection() const {
  auto collection = CollectionAnalysis::analyze(this->getCollectionOperand());
  MEMOIR_NULL_CHECK(collection,
                    "Could not determine collection of assert type");
  return *collection;
}

llvm::Value &AssertCollectionTypeInst::getCollectionOperand() const {
  return *(this->getCollectionOperandAsUse().get());
}

llvm::Use &AssertCollectionTypeInst::getCollectionOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}

std::string AssertCollectionTypeInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "AssertCollectionTypeInst: " + llvm_str;

  return str;
}

} // namespace llvm::memoir
