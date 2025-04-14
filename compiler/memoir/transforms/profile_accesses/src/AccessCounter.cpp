#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"

#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/Types.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/InternalDatatypes.hpp"

#include "folio/transforms/AccessCounter.hpp"

using namespace llvm::memoir;

using Count = uint64_t;

llvm::Type *counter_type = nullptr;

static llvm::GlobalVariable *create_counter(llvm::Module &M,
                                            llvm::StringRef name) {

  auto &context = M.getContext();
  auto *zero_init = llvm::ConstantInt::get(counter_type, 0);

  auto *global =
      new llvm::GlobalVariable(M,
                               counter_type,
                               /* constant? */ false,
                               llvm::GlobalValue::LinkageTypes::InternalLinkage,
                               zero_init,
                               name);

  MEMOIR_ASSERT(global, "Failed to create global!");

  return global;
}

static void count_accesses(AccessInst &access, Count &sparse, Count &dense) {

  // Get the type of the innermost object.
  auto *type = &access.getObjectType();
  for (auto *index : access.indices()) {

    if (auto *collection_type = dyn_cast<CollectionType>(type)) {
      type = &collection_type->getElementType();

      if (isa<SequenceType>(collection_type)) {
        ++dense;

      } else if (isa<AssocType>(collection_type)) {
        if (auto selection = collection_type->get_selection()) {
          if (selection.value() == "bitset" or selection.value() == "bitmap") {
            ++dense;
            continue;
          }
        }

        ++sparse;
      }

    } else if (auto *tuple_type = dyn_cast<TupleType>(type)) {
      auto &index_constant =
          MEMOIR_SANITIZE(dyn_cast<llvm::ConstantInt>(index),
                          "Struct field index is not constant.\n  ",
                          *index,
                          " in ",
                          access);
      auto index_value = index_constant.getZExtValue();

      type = &tuple_type->getFieldType(index_value);

    } else {
      break;
    }
  }

  return;
}

static llvm::Value *increment_counter(llvm::IRBuilder<> &builder,
                                      llvm::GlobalVariable *global,
                                      Count amount = 1) {

  // If the amount is 0, don't do anything.
  if (amount == 0) {
    return nullptr;
  }

  auto *load = builder.CreateLoad(counter_type, global);

  auto *amount_constant = llvm::ConstantInt::get(counter_type, amount);
  auto *add = builder.CreateAdd(load, amount_constant);

  auto *store = builder.CreateStore(add, global);

  return add;
}

static void create_report(llvm::Module &M,
                          llvm::ArrayRef<llvm::GlobalVariable *> globals) {

  auto &context = M.getContext();

  // Create an empty function.
  auto *void_type = llvm::Type::getVoidTy(context);
  auto *func_type = llvm::FunctionType::get(void_type, /* variadic? */ false);

  auto *func =
      llvm::Function::Create(func_type,
                             llvm::GlobalValue::LinkageTypes::InternalLinkage,
                             "memoir_report_accesses",
                             M);

  auto *block = llvm::BasicBlock::Create(context, "", func);

  auto printf_callee = llvm::FunctionCallee(M.getFunction("printf"));

  llvm::IRBuilder<> builder(block);

  bool first = true;
  std::string header_str = "";
  std::string data_str = "";
  for (auto *global : globals) {
    if (first) {
      first = false;
    } else {
      header_str += ",";
      data_str += ",";
    }
    header_str += global->getName();
    data_str += "%lu";
  }

  std::string format_str = header_str + "\n" + data_str + "\n";
  auto *format_array = llvm::ConstantDataArray::getString(context, format_str);
  auto *format_global =
      new llvm::GlobalVariable(M,
                               format_array->getType(),
                               /* constant? */ true,
                               llvm::GlobalValue::LinkageTypes::InternalLinkage,
                               format_array);

  Vector<llvm::Value *> args = { format_global };
  for (auto *global : globals) {
    auto *load = builder.CreateLoad(counter_type, global);

    args.push_back(load);
  }

  builder.CreateCall(printf_callee, args);

  builder.CreateRetVoid();

  // Record the report function in the module's destructor.
  llvm::appendToGlobalDtors(M, func, /* priority */ 10);

  return;
}

static bool transform(llvm::Module &M) {
  // Create the global variable counters.
  auto *sparse_counter = create_counter(M, "memoir_sparse_access_counter");
  auto *dense_counter = create_counter(M, "memoir_dense_access_counter");

  // Find all accesses that need to be marked.
  for (auto &F : M) {
    for (auto &BB : F) {

      // Count the local accesses.
      Count sparse = 0, dense = 0;
      for (auto &I : BB) {
        if (auto *access = into<AccessInst>(&I)) {
          count_accesses(*access, sparse, dense);
        }
      }

      // Instrument the basic block.
      llvm::IRBuilder<> builder(BB.getTerminator());
      increment_counter(builder, sparse_counter, sparse);
      increment_counter(builder, dense_counter, dense);
    }
  }

  // Create a report function to call at destruction.
  create_report(M, { sparse_counter, dense_counter });

  return true;
}

AccessCounter::AccessCounter(llvm::Module &M) {
  auto &context = M.getContext();
  counter_type = llvm::Type::getInt64Ty(context);

  transform(M);
}
