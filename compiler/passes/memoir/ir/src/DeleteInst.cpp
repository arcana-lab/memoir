#include "memoir/ir/Instructions.hpp"

#include "memoir/analysis/CollectionAnalysis.hpp"
#include "memoir/analysis/StructAnalysis.hpp"

namespace llvm::memoir {

/*
 * DeleteStructInst implementation
 */
Struct &DeleteStructInst::getStructDeleted() const {
  auto strct = StructAnalysis::analyze(this->getStructOperand());
  MEMOIR_NULL_CHECK(strct, "Could not determine the struct being deleted");
  return *strct;
}

llvm::Value &DeleteStructInst::getStructOperand() const {
  return *(this->getStructOperandAsUse().get());
}

llvm::Use &DeleteStructInst::getStructOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

std::string DeleteStructInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "DeleteStructInst: " + llvm_str;

  return str;
}

/*
 * DeleteCollectionInst implementation
 */
Collection &DeleteCollectionInst::getCollectionDeleted() const {
  auto collection =
      CollectionAnalysis::analyze(this->getCollectionOperandAsUse());
  MEMOIR_NULL_CHECK(collection,
                    "Could not determine the collection being deleted");
  return *collection;
}

llvm::Value &DeleteCollectionInst::getCollectionOperand() const {
  return *(this->getCollectionOperandAsUse().get());
}

llvm::Use &DeleteCollectionInst::getCollectionOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

std::string DeleteCollectionInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "DeleteCollectionInst: " + llvm_str;

  return str;
}

} // namespace llvm::memoir
