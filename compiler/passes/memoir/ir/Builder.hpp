#ifndef MEMOIR_BUILDER_H
#define MEMOIR_BUILDER_H
#pragma once

#include "llvm/IR/IRBuilder.h"

#include "memoir/ir/Instructions.hpp"
#include "memoir/support/Assert.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/Print.hpp"

#include "memoir/utility/FunctionNames.hpp"

namespace llvm::memoir {

class MemOIRBuilder : public IRBuilder<> {
public:
  /*
   * MemOIRBuilder Constructors
   */
  MemOIRBuilder(llvm::BasicBlock *BB) : M(BB->getModule()), IRBuilder<>(BB) {
    MEMOIR_NULL_CHECK(this->M,
                      "Could not determine LLVM Module of basic block.");
  }

  MemOIRBuilder(llvm::Instruction *IP) : M(IP->getModule()), IRBuilder<>(IP) {
    MEMOIR_NULL_CHECK(this->M,
                      "Could not determine LLVM Module of insertion point.");
  }

  /*
   * Primitive Type Instructions
   */
#define HANDLE_PRIMITIVE_TYPE_INST(ENUM, FUNC, CLASS)                          \
  CLASS *Create##CLASS(const Twine &name = "") {                               \
    auto llvm_func =                                                           \ 
      FunctionNames::get_memoir_function(*(this->M), MemOIR_Func::ENUM);       \
    auto llvm_call = this->CreateCall(llvm_func, None, name);                  \
    MEMOIR_NULL_CHECK(llvm_call,                                               \
                      "Could not construct the LLVM Call to " #FUNC);          \
    auto memoir_inst = MemOIRInst::get(*llvm_call);                            \
    auto type_inst = dyn_cast<CLASS>(memoir_inst);                             \
    return type_inst;                                                          \
  }
#include "memoir/ir/Instructions.def"

  /*
   * Derived Type Instructions
   */
  DefineStructTypeInst *CreateDefineStructTypeInst(
      const char *type_name,
      int num_fields,
      vector<llvm::Value *> field_types,
      const Twine &name = "") {
    // Fetch the LLVM Function.
    auto llvm_func =
        FunctionNames::get_memoir_function(*(this->M),
                                           MemOIR_Func::DEFINE_STRUCT_TYPE);

    // Create the LLVM type name and number of fields constant.
    auto llvm_type_name = this->CreateGlobalString(type_name, "type.struct.");
    auto llvm_num_fields = this->getInt64(num_fields);

    // Build the list of arguments.
    auto llvm_args = vector<llvm::Value *>({ llvm_type_name, llvm_num_fields });
    for (auto field_type : field_types) {
      llvm_args.push_back(field_type);
    }

    // Create the call.
    auto llvm_call = this->CreateCall(FunctionCallee(llvm_func),
                                      llvm::ArrayRef(llvm_args),
                                      name);

    // Convert to MemOIRInst and return.
    auto memoir_inst = MemOIRInst::get(*llvm_call);
    auto define_struct_type_inst = dyn_cast<DefineStructTypeInst>(memoir_inst);
    return define_struct_type_inst;
  }

  MemOIRInst *CreateStructTypeInst(const char *type_name,
                                   const Twine &name = "") {
    // Fetch the LLVM Function.
    auto llvm_func =
        FunctionNames::get_memoir_function(*(this->M),
                                           MemOIR_Func::STRUCT_TYPE);

    // TODO: Create the LLVM type name.

    // TODO: Create the call.

    // TODO: Convert to MemOIRInst and return.
    return nullptr;
  }

  // TODO: Add the other derived type instructions.

  /* TODO
   * Allocation Instructions
   */

  /* TODO
   * Access Instructions
   */

  /* TODO
   * Deletion Instructions
   */

  /* TODO
   * Collection Operation Instructions
   */
  SliceInst *CreateSliceInst(llvm::Value *collection_to_slice,
                             llvm::Value *left,
                             llvm::Value *right,
                             const Twine &name = "") {
    // Fetch the LLVM Function.
    auto llvm_func =
        FunctionNames::get_memoir_function(*(this->M), MemOIR_Func::SLICE);

    // Create the LLVM call.
    auto llvm_call =
        this->CreateCall(FunctionCallee(llvm_func),
                         llvm::ArrayRef({ collection_to_slice, left, right }),
                         name);
    MEMOIR_NULL_CHECK(llvm_call,
                      "Could not create the call for slice operation.");

    // Cast to MemOIRInst and return.
    auto memoir_inst = MemOIRInst::get(*llvm_call);
    auto slice_inst = dyn_cast<SliceInst>(memoir_inst);
    MEMOIR_NULL_CHECK(slice_inst, "Could not create call to SliceInst");
    return slice_inst;
  }

  SliceInst *CreateSliceInst(llvm::Value *collection_to_slice,
                             llvm::Value *left,
                             int64_t right,
                             const Twine &name = "") {
    // Create the right constant.
    auto right_constant = this->getInt64(right);

    // Call the base builder method.
    return this->CreateSliceInst(collection_to_slice,
                                 left,
                                 right_constant,
                                 name);
  }

  SliceInst *CreateSliceInst(llvm::Value *collection_to_slice,
                             int64_t left,
                             llvm::Value *right,
                             const Twine &name = "") {
    // Create the left constant.
    auto left_constant = this->getInt64(left);

    // Call the base builder method.
    return this->CreateSliceInst(collection_to_slice,
                                 left_constant,
                                 right,
                                 name);
  }

  SliceInst *CreateSliceInst(llvm::Value *collection_to_slice,
                             int64_t left,
                             int64_t right,
                             const Twine &name = "") {
    // Create the left constant.
    auto left_constant = this->getInt64(left);

    // Create the right constant.
    auto right_constant = this->getInt64(right);

    // Call the base builder method.
    return this->CreateSliceInst(collection_to_slice,
                                 left_constant,
                                 right_constant,
                                 name);
  }

  JoinInst *CreateJoinInst(vector<llvm::Value *> collections_to_join,
                           const Twine &name = "") {
    // Fetch the LLVM Function.
    auto llvm_func =
        FunctionNames::get_memoir_function(*(this->M), MemOIR_Func::JOIN);

    // Create the LLVM argument list.
    auto num_joins = collections_to_join.size();
    MEMOIR_ASSERT(num_joins <= 255,
                  "Attempt to join more that 255 collection!");
    auto llvm_num_joins = this->getInt8(num_joins);
    vector<llvm::Value *> llvm_args{ llvm_num_joins };
    for (auto collection : collections_to_join) {
      llvm_args.push_back(collection);
    }

    // Create the LLVM call.
    auto llvm_call =
        this->CreateCall(FunctionCallee(llvm_func), llvm_args, name);

    // Cast to MemOIRInst and return.
    auto memoir_inst = MemOIRInst::get(*llvm_call);
    auto join_inst = dyn_cast<JoinInst>(memoir_inst);
    MEMOIR_NULL_CHECK(join_inst, "Could not create the call to JoinInst");
    return join_inst;
  }

protected:
  // Borrowed state
  llvm::Module *M;

  // Helper Functions
};

} // namespace llvm::memoir

#endif
