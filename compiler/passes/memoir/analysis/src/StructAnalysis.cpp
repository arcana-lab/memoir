#include "memoir/analysis/StructAnalysis.hpp"

namespace llvm::memoir {

/*
 * Initialization
 */
StructAnalysis::StructAnalysis() {
  // Do nothing.
}

/*
 * Queries
 */
Struct *StructAnalysis::analyze(llvm::Value &V) {
  return StructAnalysis::get().getStruct(V);
}

/*
 * Analysis
 */
Struct *StructAnalysis::getStruct(llvm::Value &V) {
  /*
   * Find the associated memoir struct, if it exists.
   */

  /*
   * If we have an instruction, visit it.
   */
  if (auto inst = dyn_cast<llvm::Instruction>(&V)) {
    return this->visit(*inst);
  }

  /*
   * Handle function arguments by constructing an ArgPHI
   */
  if (auto arg = dyn_cast<llvm::Argument>(&V)) {
    return this->visitArgument(*arg);
  }

  return nullptr;
}

/*
 * Helper macros
 */
#define CHECK_MEMOIZED(V)                                                      \
  /* See if an existing struct exists, if it does, early return. */            \
  if (auto found = this->findExisting(V)) {                                    \
    return found;                                                              \
  }

#define MEMOIZE_AND_RETURN(V, S)                                               \
  /* Memoize the struct */                                                     \
  this->memoize(V, S);                                                         \
  /* Return */                                                                 \
  return S

/*
 * Visitor functions
 */
Struct *StructAnalysis::visitInstruction(llvm::Instruction &I) {
  CHECK_MEMOIZED(I);

  // Could not find an interesting instruction, return NULL.

  MEMOIZE_AND_RETURN(I, nullptr);
}

Struct *StructAnalysis::visitStructAllocInst(StructAllocInst &I) {
  CHECK_MEMOIZED(I);

  auto base_struct = new BaseStruct(I);

  MEMOIZE_AND_RETURN(I, base_struct);
}

Struct *StructAnalysis::visitStructGetInst(StructGetInst &I) {
  CHECK_MEMOIZED(I);

  // TODO: add type checking to the GetInst to make sure it's a struct

  auto nested_struct = new NestedStruct(I);

  MEMOIZE_AND_RETURN(I, nested_struct);
}

Struct *StructAnalysis::visitIndexGetInst(IndexGetInst &I) {
  CHECK_MEMOIZED(I);

  // TODO: add type checking to the GetInst to make sure it's a struct

  auto contained_struct = new ContainedStruct(I);

  MEMOIZE_AND_RETURN(I, contained_struct);
}

Struct *StructAnalysis::visitAssocGetInst(AssocGetInst &I) {
  CHECK_MEMOIZED(I);

  // TODO: add type checking to the GetInst to make sure it's a struct

  auto contained_struct = new ContainedStruct(I);

  MEMOIZE_AND_RETURN(I, contained_struct);
}

Struct *StructAnalysis::visitReadInst(ReadInst &I) {
  CHECK_MEMOIZED(I);

  // TODO: perform type checking on the ReadInst collection to make sure its a
  // struct reference

  MEMOIZE_AND_RETURN(I, nullptr);
}

Struct *StructAnalysis::visitLLVMCallInst(llvm::CallInst &I) {
  CHECK_MEMOIZED(I);

  vector<llvm::ReturnInst *> incoming_returns = {};
  map<llvm::ReturnInst *, Struct *> incoming = {};

  auto callee = I.getCalledFunction();
  if (callee) {
    // Handle direct call.
    for (auto &BB : *callee) {
      auto terminator = BB.getTerminator();
      if (auto return_inst = dyn_cast<ReturnInst>(terminator)) {
        auto incoming_struct = this->visitReturnInst(*return_inst);
        incoming_returns.push_back(return_inst);
        incoming[return_inst] = incoming_struct;
      }
    }
  } else {
    // Handle indirect call.
    auto function_type = I.getFunctionType();
    MEMOIR_NULL_CHECK(function_type, "Found a call with NULL function type");

    auto parent_bb = I.getParent();
    MEMOIR_NULL_CHECK(parent_bb,
                      "Could not determine the parent basic block of the call");
    auto parent_function = parent_bb->getParent();
    MEMOIR_NULL_CHECK(parent_function,
                      "Could not determine the parent function of the call");
    auto parent_module = parent_function->getParent();
    MEMOIR_NULL_CHECK(parent_module,
                      "Could not determine the parent module of the call");

    for (auto &F : *parent_module) {
      if (F.getFunctionType() != function_type) {
        continue;
      }

      for (auto &BB : F) {
        auto terminator = BB.getTerminator();
        if (auto return_inst = dyn_cast<ReturnInst>(terminator)) {
          auto incoming_struct = this->visitReturnInst(*return_inst);
          incoming_returns.push_back(return_inst);
          incoming[return_inst] = incoming_struct;
        }
      }
    }
  }

  auto return_phi_struct = new RetPHIStruct(I, incoming_returns, incoming);

  MEMOIZE_AND_RETURN(I, return_phi_struct);
}

Struct *StructAnalysis::visitPHINode(llvm::PHINode &I) {
  CHECK_MEMOIZED(I);

  map<llvm::BasicBlock *, Struct *> incoming = {};

  for (auto bb : I.blocks()) {
    auto incoming_value = I.getIncomingValueForBlock(bb);
    auto incoming_struct = this->getStruct(*incoming_value);
    MEMOIR_NULL_CHECK(incoming_struct,
                      "Could not determine incoming struct for Control PHI");

    incoming[bb] = incoming_struct;
  }

  auto control_phi_struct = new ControlPHIStruct(I, incoming);

  MEMOIZE_AND_RETURN(I, control_phi_struct);
}

Struct *StructAnalysis::visitReturnInst(llvm::ReturnInst &I) {
  CHECK_MEMOIZED(I);

  auto return_value = I.getReturnValue();
  MEMOIR_NULL_CHECK(return_value, "Return value is NULL!");

  auto return_struct = this->getStruct(*return_value);

  MEMOIZE_AND_RETURN(I, return_struct);
}

Struct *StructAnalysis::visitArgument(llvm::Argument &A) {
  CHECK_MEMOIZED(A);

  vector<llvm::CallBase *> incoming_calls;
  map<llvm::CallBase *, Struct *> incoming;
  auto parent_function = A.getParent();
  MEMOIR_NULL_CHECK(parent_function,
                    "Could not determine the parent function of the Argument");
  auto parent_module = parent_function->getParent();
  MEMOIR_NULL_CHECK(parent_module,
                    "Could not determine the parent module of the Argument");

  auto parent_function_type = parent_function->getFunctionType();
  MEMOIR_NULL_CHECK(parent_function_type,
                    "Function type of Argument's parent function is NULL!");

  for (auto &F : *parent_module) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        auto call_base = dyn_cast<llvm::CallBase>(&I);

        // Ignore non-call instructions.
        if (!call_base) {
          continue;
        }

        // Check the called function.
        auto callee = call_base->getCalledFunction();
        if (callee == parent_function) {
          // Handle direct calls.
          auto call_argument = call_base->getArgOperand(A.getArgNo());
          auto argument_struct = this->getStruct(*call_argument);
          MEMOIR_NULL_CHECK(argument_struct,
                            "Incoming struct to Argument is NULL!");

          incoming_calls.push_back(call_base);
          incoming[call_base] = argument_struct;
        } else if (!callee) {
          // Handle indirect calls.
          auto function_type = callee->getFunctionType();
          if (function_type == parent_function_type) {
            auto call_argument = call_base->getArgOperand(A.getArgNo());
            auto argument_struct = this->getStruct(*call_argument);
            MEMOIR_NULL_CHECK(argument_struct,
                              "Incoming struct to Argument is NULL!");

            incoming_calls.push_back(call_base);
            incoming[call_base] = argument_struct;
          }
        }
      }
    }
  }

  auto argument_phi_struct = new ArgPHIStruct(A, incoming_calls, incoming);

  MEMOIZE_AND_RETURN(A, argument_phi_struct);
}

/*
 * Internal helper functions
 */
Struct *StructAnalysis::findExisting(llvm::Value &V) {
  auto found_struct = this->value_to_struct.find(&V);
  if (found_struct != this->value_to_struct.end()) {
    return found_struct->second;
  }

  return nullptr;
}

Struct *StructAnalysis::findExisting(MemOIRInst &I) {
  return this->findExisting(I.getCallInst());
}

void StructAnalysis::memoize(llvm::Value &V, Struct *S) {
  this->value_to_struct[&V] = S;
}

void StructAnalysis::memoize(MemOIRInst &I, Struct *S) {
  this->memoize(I.getCallInst(), S);
}

/*
 * Management
 */
StructAnalysis *StructAnalysis::SA = nullptr;

StructAnalysis &StructAnalysis::get() {
  if (StructAnalysis::SA == nullptr) {
    StructAnalysis::SA = new StructAnalysis();
  }
  return *(StructAnalysis::SA);
}

void StructAnalysis::invalidate() {
  StructAnalysis::get()._invalidate();
  return;
}

void StructAnalysis::_invalidate() {
  // TODO: delete all structs in value_to_struct
  this->value_to_struct.clear();
  return;
}

} // namespace llvm::memoir
