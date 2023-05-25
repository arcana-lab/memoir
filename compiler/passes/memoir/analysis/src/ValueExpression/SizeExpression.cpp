#include "memoir/analysis/ValueExpression.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

namespace llvm::memoir {

bool SizeExpression::isAvailable(llvm::Instruction &IP,
                                 const llvm::DominatorTree *DT,
                                 llvm::CallBase *call_context) {
  println("Materializing ", *this);
  if (!CE) {
    println("Could not find the collection expression being sized");
    return false;
  }

  return CE->isAvailable(IP, DT, call_context);
}

llvm::Value *SizeExpression::materialize(llvm::Instruction &IP,
                                         MemOIRBuilder *builder,
                                         const llvm::DominatorTree *DT,
                                         llvm::CallBase *call_context) {
  println("Materializing ", *this);
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

  // Forward along any function versioning.
  this->function_version = CE->function_version;
  this->version_mapping = CE->version_mapping;
  this->call_version = CE->call_version;

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
