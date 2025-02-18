#include "TypeInference.hpp"

#include "memoir/support/Casting.hpp"

#include "llvm/IR/CFG.h"

namespace llvm::memoir {

bool TypeInference::run() {
  if (this->infer(this->M)) {
    return this->annotate(this->M);
  }

  return false;
}

// Inferred type
using inferred_type = tuple<bool, Type *>;

static inferred_type argument_has_type_annotation(llvm::Argument &A) {

  // Quickly check that the argument has pointer type.
  if (not isa<llvm::PointerType>(A.getType())) {
    return { false, nullptr };
  }

  // For each use, see if it is used by an Assert*TypeInst.
  for (auto &uses : A.uses()) {
    auto *user = uses.getUser();
    auto *user_as_inst = dyn_cast<llvm::Instruction>(user);

    // If the user isn't an instruction, continue.
    if (user_as_inst == nullptr) {
      continue;
    }

    // Check for a type assertion.
    if (auto *assert_inst = into<AssertTypeInst>(user_as_inst)) {
      return { true, &assert_inst->getType() };
    }
  }

  // If we couldn't find a type annotation for the argument, return false.
  return { false, nullptr };
}

bool TypeInference::infer_argument_type(llvm::Argument &A) {
  // See if the argument has a type annotation.
  // If it does, continue.
  auto [annotated, annotated_type] = argument_has_type_annotation(A);
  if (annotated) {
    return true;
  }

  // Otherwise, we need to infer the type.

  // See if the type checker can get the type for us.
  if (auto *type = type_of(A)) {
    // It worked! Mark the argument type to be annotated and continue.
    this->argument_types_to_annotate[&A] = type;
    return true;
  }

  // Guess it's time to break out our own analysis.

  // First, check to see if the function is in a fold instruction, if so, we can
  // use type information there.
  auto &F = MEMOIR_SANITIZE(A.getParent(), "Argument has NULL parent!");
  for (auto &use : F.uses()) {
    auto *user = use.getUser();
    if (auto *inst = dyn_cast<llvm::Instruction>(user)) {
      if (auto *fold = into<FoldInst>(inst)) {

        // If we are not the function operand, continue.
        if (&fold->getBody() != &F) {
          continue;
        }

        // Determine the type of the value based on the operand(s) to the fold.
        Type *type = nullptr;
        bool found_type = false;

        // Fetch the collection type.
        auto &collection_type = MEMOIR_SANITIZE(
            dyn_cast_or_null<CollectionType>(&fold->getElementType()),
            "Fold over non-collection type!\n  ",
            *fold);

        // Get the element type.
        auto &element_type = collection_type.getElementType();

        // If we are the accumulator argument, we are the initial value's type.
        if (A.getArgNo() == 0) {

          // Otherwise, get the type of the initial accumulator value.
          type = type_of(fold->getInitial());
          found_type = true;
        }

        // If we are the key argument, we are the key type.
        else if (A.getArgNo() == 1) {

          // If the collection is an assoc type, get the key type.
          if (auto *assoc_type = dyn_cast<AssocArrayType>(&collection_type)) {
            auto &key_type = assoc_type->getKeyType();

            type = &key_type;
            found_type = true;
          }
          // Otherwise, it's an index type, and therefore an LLVM type.
          else {
            type = nullptr;
            found_type = true;
          }
        }

        // If the element type is non-void, and we are the value argument, we
        // are the element type.
        else if (not isa<VoidType>(&element_type) and A.getArgNo() == 2) {
          type = &element_type;
          found_type = true;
        }

        // Otherwise, get the closure argument that we match with.
        else if (auto closed_kw = fold->get_keyword<ClosedKeyword>()) {

          auto first_closed_argument = isa<VoidType>(&element_type) ? 2 : 3;

          auto index = A.getArgNo() - first_closed_argument;

          auto &closed_value = MEMOIR_SANITIZE(
              *std::next(closed_kw->args_begin(), index),
              "Failed to fetch the closed value for fold function argument");

          type = type_of(closed_value);
          found_type = true;
        }

        // If we found a type:
        if (found_type) {

          // If it's a collection or struct type, mark it for annotation.
          if (isa_and_nonnull<CollectionType>(type)
              || isa_and_nonnull<StructType>(type)) {
            this->argument_types_to_annotate[&A] = type;
            return true;
          }

          // Otherwise, we succeed, but return NULL because it is an LLVM type.
          return true;
        }
      }
    }
  }

  // Second, let's check the callers.
  for (auto &use : F.uses()) {
    // Get the user information.
    auto *user = use.getUser();
    auto *user_as_inst = dyn_cast_or_null<llvm::Instruction>(user);

    // If the user is a memoir instruction, skip it.
    if (into<MemOIRInst>(user_as_inst)) {
      continue;
    }

    // If the user is a call instruction, recurse on it's parent function.
    if (auto *user_as_call = dyn_cast_or_null<llvm::CallBase>(user_as_inst)) {
      // Recurse on the caller function.
      if (auto *caller_bb = user_as_call->getParent()) {
        if (auto *caller_function = caller_bb->getParent()) {

          // Only recurse if we are looking at a different function.
          if (caller_function != &F) {
            // TODO: replace this with type unification.
            this->infer(*caller_function);
          }
        }
      }

      // Then, see if we can type the operand being passed into the call
      // for this argument.
      auto arg_index = A.getArgNo();
      auto &call_operand =
          MEMOIR_SANITIZE(user_as_call->getArgOperand(arg_index),
                          "Operand of call is NULL!");
      if (auto *call_operand_type = type_of(call_operand)) {
        // It worked! Mark the argument type to be annotated and continue;
        this->argument_types_to_annotate[&A] = call_operand_type;
        return true;
      }
    }
  }

  return false;
}

static inferred_type function_has_return_type_annotation(llvm::Function &F) {
  // Check the LLVM return type of the function.
  auto *return_type = F.getReturnType();

  // If it either doesn't have a return type, or is not a MEMOIR collection or
  // struct, we succeed.
  if (return_type == nullptr || not isa<llvm::PointerType>(return_type)) {
    return { true, nullptr };
  }

  // For each instruction in the function, check if it is a return type
  // annotation.
  for (auto &BB : F) {
    for (auto &I : BB) {
      // If there is a return type annotation, return true.
      if (auto *return_type_inst = into<ReturnTypeInst>(I)) {
        return { true, &return_type_inst->getType() };
      }
    }
  }

  // If we got here, then we couldn't find a type annotation!
  return { false, nullptr };
}

bool TypeInference::infer_return_type(llvm::Function &F) {
  // See if the argument has a type annotation.
  // If it does, continue.
  auto [annotated, annotated_type] = function_has_return_type_annotation(F);
  if (annotated) {
    return true;
  }

  // Otherwise, we need to infer the type from the return instructions.
  set<llvm::Value *> returned_values = {};
  for (auto &BB : F) {
    // Get the return instruction.
    auto *return_inst = dyn_cast_or_null<llvm::ReturnInst>(BB.getTerminator());
    if (return_inst == nullptr) {
      continue;
    }

    // Get the return value.
    auto *return_value = return_inst->getReturnValue();
    if (return_value == nullptr) {
      continue;
    }
  }

  Type *unified_type = nullptr;
  for (auto *returned : returned_values) {

    // See if TypeAnalysis can get the type for us.
    if (auto *returned_type = type_of(*returned)) {
      // If the unified type is undefined, set it to the returned type.
      if (unified_type == nullptr) {
        unified_type = returned_type;
      }
      // Otherwise, if the unified type differs from the returned type, error!
      else if (unified_type != returned_type) {
        MEMOIR_UNREACHABLE("TYPE ERROR: Overdefined return type!");
      }
      // Otherwise, we are good to keep going.
    }
  }

  // If we were able to unify the type, mark the return type to annotate.
  if (unified_type != nullptr) {
    this->return_types_to_annotate[&F] = unified_type;
    return true;
  }

  // Check to see if the function is in a fold instruction, if so, we can use
  // type information there.
  for (auto &use : F.uses()) {
    auto *user = use.getUser();
    if (auto *inst = dyn_cast<llvm::Instruction>(user)) {
      if (auto *fold = into<FoldInst>(inst)) {
        // If we are not the function operand, continue.
        if (&fold->getBody() != &F) {
          continue;
        }

        // Otherwise, get the type of the initial accumulator value.
        auto *accumulator_type = type_of(fold->getInitial());

        // If the accumulator type is a collection or struct type, return it.
        if (isa<CollectionType>(accumulator_type)
            || isa<StructType>(accumulator_type)) {
          this->return_types_to_annotate[&F] = accumulator_type;
          return true;
        }

        // Otherwise, we succeeded, but the return type is an LLVM type.
        return true;
      }
    }
  }

  // Otherwise, we did not infer a memoir type for the return.
  return false;
}

bool TypeInference::infer(llvm::Function &F) {
  // Infer the type of function arguments.
  for (auto &A : F.args()) {
    infer_argument_type(A);
  }

  // Infer the type of function returns.
  infer_return_type(F);

  return true;
}

bool TypeInference::infer(llvm::Module &M) {
  for (auto &F : M) {
    // Analyze the function.
    this->infer(F);
  }

  return true;
}

// Transformation.
void TypeInference::annotate_argument_type(llvm::Argument &A, Type &type) {
  // Get the parent function of the argument.
  auto &F = MEMOIR_SANITIZE(A.getParent(), "Argument has no parent function!");
  if (F.empty()) {
    return;
  }

  // Create a builder.
  auto &entry_bb = F.getEntryBlock();
  auto *entry_insertion_point = entry_bb.getFirstNonPHI();
  MemOIRBuilder builder(entry_insertion_point);

  // Create a type annotation for the argument.
  builder.CreateAssertTypeInst(&A, type, "type.infer.");

  return;
}

void TypeInference::annotate_return_type(llvm::Function &F, Type &type) {
  if (F.empty()) {
    return;
  }

  // Create a builder.
  auto &entry_bb = F.getEntryBlock();
  auto *entry_insertion_point = entry_bb.getFirstNonPHI();
  MemOIRBuilder builder(entry_insertion_point);

  // Create a type annotation for the argument.
  builder.CreateReturnTypeInst(type, "type.infer.");

  return;
}

bool TypeInference::annotate(llvm::Module &M) {
  bool transformed = false;

  // Annotate each of the argument types.
  for (auto const [argument, type] : argument_types_to_annotate) {
    debugln("Annotating ",
            *argument,
            " in ",
            argument->getParent()->getName(),
            " of type ",
            *type);
    annotate_argument_type(*argument, *type);
  }

  // Annotate each of the return types.
  for (auto const [function, type] : return_types_to_annotate) {
    debugln("Annotating ", function->getName(), " return of type ", *type);
    annotate_return_type(*function, *type);
  }

  return transformed;
}

} // namespace llvm::memoir
