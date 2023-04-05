#include "memoir/analysis/ValueExpression.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

namespace llvm::memoir {

// ConstantExpression.
bool ConstantExpression::isAvailable(llvm::Instruction &IP,
                                     const llvm::DominatorTree *DT,
                                     llvm::CallBase *call_context) const {
  return true;
}

llvm::Value *ConstantExpression::materialize(
    llvm::Instruction &IP,
    MemOIRBuilder *builder,
    const llvm::DominatorTree *DT,
    llvm::CallBase *call_context) const {
  return &C;
}

// VariableExpression
bool VariableExpression::isAvailable(llvm::Instruction &IP,
                                     const llvm::DominatorTree *DT,
                                     llvm::CallBase *call_context) const {
  return false;
}

llvm::Value *VariableExpression::materialize(
    llvm::Instruction &IP,
    MemOIRBuilder *builder,
    const llvm::DominatorTree *DT,
    llvm::CallBase *call_context) const {
  return nullptr;
}

// ArgumentExpression
bool ArgumentExpression::isAvailable(llvm::Instruction &IP,
                                     const llvm::DominatorTree *DT,
                                     llvm::CallBase *call_context) const {
  // Get the insertion basic block and function.
  auto insertion_bb = IP.getParent();
  if (!insertion_bb) {
    return false;
  }
  auto insertion_func = insertion_bb->getParent();
  if (!insertion_func) {
    return false;
  }

  // Check that the insertion point and the argument are in the same function.
  auto value_func = this->A.getParent();
  if (value_func == insertion_func) {
    return true;
  }

  // Otherwise, the argument is not available.
  return false;
}

llvm::Value *ArgumentExpression::materialize(
    llvm::Instruction &IP,
    MemOIRBuilder *builder,
    const llvm::DominatorTree *DT,
    llvm::CallBase *call_context) const {
  if (isAvailable(IP, DT, call_context)) {
    return &A;
  }
  return nullptr;
}

// UnknownExpression
bool UnknownExpression::isAvailable(llvm::Instruction &IP,
                                    const llvm::DominatorTree *DT,
                                    llvm::CallBase *call_context) const {
  return false;
}

llvm::Value *UnknownExpression::materialize(
    llvm::Instruction &IP,
    MemOIRBuilder *builder,
    const llvm::DominatorTree *DT,
    llvm::CallBase *call_context) const {
  return nullptr;
}

// BasicExpression
bool BasicExpression::isAvailable(llvm::Instruction &IP,
                                  const llvm::DominatorTree *DT,
                                  llvm::CallBase *call_context) const {
  // Get the insertion basic block and function.
  auto insertion_bb = IP.getParent();
  if (!insertion_bb) {

    return false;
  }
  auto insertion_func = insertion_bb->getParent();
  if (!insertion_func) {
    return false;
  }

  // If this Instruction exists, check if it dominates the insertion point.
  if ((this->I != nullptr) && (DT != nullptr)) {
    if (DT->dominates(this->I, &IP)) {
      return true;
    }
  }

  // If this Instruction exists, check that the instruction has no side
  // effects.
  if (this->I != nullptr) {
    if (this->I->mayHaveSideEffects()) {
      return false;
    }
  }

  // Otherwise, iterate on the arguments of this expression.
  bool is_available = true;
  for (auto arg_idx = 0; arg_idx < this->getNumArguments(); arg_idx++) {
    auto arg = this->getArgument(arg_idx);
    MEMOIR_NULL_CHECK(arg,
                      "BasicExpression has a NULL expression as an argument");
    is_available &= arg->isAvailable(IP, DT, call_context);
  }
  return is_available;
}

llvm::Value *BasicExpression::materialize(llvm::Instruction &IP,
                                          MemOIRBuilder *builder,
                                          const llvm::DominatorTree *DT,
                                          llvm::CallBase *call_context) const {
  // TODO
  return nullptr;
}

// PHIExpression
llvm::Value *PHIExpression::materialize(llvm::Instruction &IP,
                                        MemOIRBuilder *builder,
                                        const llvm::DominatorTree *DT,
                                        llvm::CallBase *call_context) const {
  // TODO
  return nullptr;
}

// CallExpression
llvm::Value *CallExpression::materialize(llvm::Instruction &IP,
                                         MemOIRBuilder *builder,
                                         const llvm::DominatorTree *DT,
                                         llvm::CallBase *call_context) const {
  // TODO
  return nullptr;
}

// CollectionExpression
bool CollectionExpression::isAvailable(llvm::Instruction &IP,
                                       const llvm::DominatorTree *DT,
                                       llvm::CallBase *call_context) const {

  return false;
}

llvm::Value *CollectionExpression::materialize(
    llvm::Instruction &IP,
    MemOIRBuilder *builder,
    const llvm::DominatorTree *DT,
    llvm::CallBase *call_context) const {
  // TODO
  return nullptr;
}

// StructExpression
bool StructExpression::isAvailable(llvm::Instruction &IP,
                                   const llvm::DominatorTree *DT,
                                   llvm::CallBase *call_context) const {

  return false;
}

llvm::Value *StructExpression::materialize(llvm::Instruction &IP,
                                           MemOIRBuilder *builder,
                                           const llvm::DominatorTree *DT,
                                           llvm::CallBase *call_context) const {
  // TODO
  return nullptr;
}

//  SizeExpression
bool SizeExpression::isAvailable(llvm::Instruction &IP,
                                 const llvm::DominatorTree *DT,
                                 llvm::CallBase *call_context) const {
  if (!CE) {
    println("Could not find the collection expression being sized");
    return false;
  }

  return CE->isAvailable(IP, DT, call_context);
}

llvm::Value *SizeExpression::materialize(llvm::Instruction &IP,
                                         MemOIRBuilder *builder,
                                         const llvm::DominatorTree *DT,
                                         llvm::CallBase *call_context) const {
  if (!CE) {
    return nullptr;
  }

  bool created_builder = false;
  if (!builder) {
    builder = new MemOIRBuilder(&IP);
    created_builder = true;
  }

  // Materialize the collection.
  auto materialized_collection = CE->materialize(IP, builder, DT, call_context);

  // If we couldn't materialize the collection, cleanup and return NULL.
  if (!materialized_collection) {
    if (created_builder) {
      delete builder;
    }
    return nullptr;
  }

  // Materialize the call to size.
  auto materialized_size = builder->CreateSizeInst(materialized_collection);

  // Return the LLVM Value that was materialized.
  return &(materialized_size->getCallInst());
}

} // namespace llvm::memoir
