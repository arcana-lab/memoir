#include "llvm/ADT/ArrayRef.h"

#include "memoir/ir/Builder.hpp"
#include "memoir/transforms/utilities/MutateType.hpp"
#include "memoir/utility/FunctionNames.hpp"

namespace llvm::memoir {

#if 0
  
// We will use two special offset values to represent either the keys or
// elements at a given offset.
#  define ELEMS unsigned(-1)
#  define KEYS unsigned(-2)

enum DifferenceKind : uint8_t {
  NoDifference = 0,
  TypeDiffers = 1 << 0,
  SelectionDiffers = 1 << 1,
};

inline DifferenceKind operator|(DifferenceKind lhs, DifferenceKind rhs) {
  return static_cast<DifferenceKind>(static_cast<uint8_t>(lhs)
                                     | static_cast<uint8_t>(rhs));
}

inline DifferenceKind operator&(DifferenceKind lhs, DifferenceKind rhs) {
  return static_cast<DifferenceKind>(static_cast<uint8_t>(lhs)
                                     & static_cast<uint8_t>(rhs));
}

inline DifferenceKind &operator|=(DifferenceKind &lhs, DifferenceKind rhs) {
  return (DifferenceKind &)((uint8_t &)(lhs) |= static_cast<uint8_t>(rhs));
}

inline DifferenceKind &operator&=(DifferenceKind &lhs, DifferenceKind rhs) {
  return (DifferenceKind &)((uint8_t &)(lhs) &= static_cast<uint8_t>(rhs));
}

struct Difference {
public:
  Difference(DifferenceKind kind, llvm::ArrayRef<unsigned> offsets)
    : _kind(kind),
      _offsets(offsets.begin(), offsets.end()) {}

  bool type_differs() const {
    return (this->_kind & DifferenceKind::TypeDiffers)
           != DifferenceKind::NoDifference;
  }

  bool selection_differs() const {
    return (this->_kind & DifferenceKind::SelectionDiffers)
           != DifferenceKind::NoDifference;
  }

  llvm::ArrayRef<unsigned> offsets() const {
    return this->_offsets;
  }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const Difference &diff) {
    os << "DIFFERS:";
    if (diff.type_differs()) {
      os << " type";
    }
    if (diff.selection_differs()) {
      os << " selection";
    }
    os << "\n";

    os << "  ";
    for (auto offset : diff.offsets()) {
      switch (offset) {
        case KEYS:
          os << ".keys";
          break;
        case ELEMS:
          os << "[*]";
          break;
        default:
          os << "." << std::to_string(offset);
          break;
      }
    }

    return os;
  }

protected:
  DifferenceKind _kind;
  vector<unsigned> _offsets;
};

struct Differences : public vector<Difference> {
  Differences() {}

  void add(DifferenceKind kind, llvm::ArrayRef<unsigned> offsets) {
    this->emplace_back(kind, offsets);
  }
};

static void type_differences(Differences &differences,
                             llvm::ArrayRef<unsigned> offsets,
                             Type &base,
                             Type &other) {

  // If the types are the same, continue.
  if (base == other) {
    return;
  }

  // If both of the types are non-equal primitive types, add the difference.
  if (Type::is_primitive_type(base)) {
    MEMOIR_ASSERT(Type::is_primitive_type(other),
                  "Non-isomorphic type mutation! ",
                  base,
                  " => ",
                  other);

    differences.emplace_back(DifferenceKind::TypeDiffers, offsets);

    return;
  }

  // If these are the same type kind, recurse on their inner types.
  if (base.getKind() == other.getKind()) {

    if (auto *base_tuple = dyn_cast<TupleType>(&base)) {
      auto *other_tuple = cast<TupleType>(&other);

      auto base_fields = base_tuple->getNumFields();
      auto other_fields = other_tuple->getNumFields();
      MEMOIR_ASSERT(base_fields == other_fields,
                    "Non-isomorphic type mutation! ",
                    base,
                    " => ",
                    other);

      vector<unsigned> nested_offsets(offsets.begin(), offsets.end());
      for (unsigned field = 0; field < base_fields; ++field) {
        nested_offsets.push_back(field);

        type_differences(differences,
                         nested_offsets,
                         base_tuple->getFieldType(field),
                         other_tuple->getFieldType(field));

        nested_offsets.pop_back();
      }

      return;

    } else if (auto *base_assoc = dyn_cast<AssocType>(&base)) {
      auto *other_assoc = cast<AssocType>(&other);

      // Check differences on the key.
      auto &base_key = base_assoc->getKeyType();
      auto &other_key = other_assoc->getKeyType();

      vector<unsigned> nested_offsets(offsets.begin(), offsets.end());

      nested_offsets.push_back(-2);
      type_differences(differences, nested_offsets, base_key, other_key);
      nested_offsets.pop_back();

      // Check differences on the element.
      auto &base_elem = base_assoc->getElementType();
      auto &other_elem = other_assoc->getElementType();

      nested_offsets.push_back(-1);
      type_differences(differences, nested_offsets, base_elem, other_elem);
      nested_offsets.pop_back();

      // Check differences on the selection.
      if (base_assoc->get_selection() != other_assoc->get_selection()) {
        differences.add(DifferenceKind::SelectionDiffers, offsets);
      }

      return;

    } else if (auto *base_seq = dyn_cast<SequenceType>(&base)) {
      auto *other_seq = cast<SequenceType>(&other);

      auto &base_elem = base_seq->getElementType();
      auto &other_elem = other_seq->getElementType();

      vector<unsigned> nested_offsets(offsets.begin(), offsets.end());

      nested_offsets.push_back(-1);
      type_differences(differences, nested_offsets, base_elem, other_elem);
      nested_offsets.pop_back();

      // Check differences on the selection.
      if (base_seq->get_selection() != other_seq->get_selection()) {
        differences.add(DifferenceKind::SelectionDiffers, offsets);
      }

      return;
    }
  }

  // If we are switching from assoc -> sequence

  // Or from sequence -> assoc

  // Otherwise, the mutation is not isomorphic, unhandled!
  // Do it yourself!
  MEMOIR_UNREACHABLE("Non-isomorphic type mutation! ", base, " => ", other);

  return;
}

static vector<Difference> type_differences(Type &base, Type &other) {
  Differences differences = {};

  type_differences(differences, {}, base, other);

  return differences;
}

static void gather_base_redefinitions(set<llvm::Value *> &redefs,
                                      llvm::Value &V) {

  if (redefs.count(&V) > 0) {
    return;
  }

  redefs.insert(&V);

  for (auto &use : V.uses()) {
    auto *user = dyn_cast<llvm::Instruction>(use.getUser());
    if (not user) {
      continue;
    }

    if (auto *memoir = into<MemOIRInst>(user)) {

      if (auto *update = dyn_cast<UpdateInst>(memoir)) {
        if (update->getObjectAsUse() == use) {
          gather_base_redefinitions(redefs, update->getResult());
        }

      } else if (auto *fold = dyn_cast<FoldInst>(memoir)) {
        if (use == fold->getInitialAsUse()) {
          gather_base_redefinitions(redefs, fold->getAccumulatorArgument());
          gather_base_redefinitions(redefs, fold->getResult());

        } else if (auto *closed = fold->getClosedArgument(use)) {
          gather_base_redefinitions(redefs, *closed);
        }

      } else if (auto *ret_phi = dyn_cast<RetPHIInst>(memoir)) {
        gather_base_redefinitions(redefs, ret_phi->getResult());
      }

    } else if (auto *phi = dyn_cast<llvm::PHINode>(user)) {
      gather_base_redefinitions(redefs, *phi);
    } else if (auto *call = dyn_cast<llvm::CallBase>(user)) {
      auto *callee = call->getCalledFunction();
      MEMOIR_ASSERT(callee,
                    "Object passed into indirect call! ",
                    value_name(V),
                    " in ",
                    *call);

      auto operand_no = use.getOperandNo();
      MEMOIR_ASSERT(operand_no < callee->arg_size(),
                    "Object passed to argument out of range! ",
                    value_name(V),
                    " in ",
                    *call);

      auto &arg = MEMOIR_SANITIZE(callee->getArg(operand_no),
                                  "Argument ",
                                  operand_no,
                                  " in ",
                                  callee->getName(),
                                  " is NULL!");

      gather_base_redefinitions(redefs, arg);
    }
  }
}

static void update_assertions(llvm::Value &V, Type &type) {
  for (auto &use : V.uses()) {
    if (auto *assertion = into<AssertTypeInst>(use.getUser())) {
      MemOIRBuilder builder(*assertion);

      auto *type_value = &builder.CreateTypeInst(type)->getCallInst();

      assertion->getTypeOperandAsUse().set(type_value);

      println("Updated assertion for ",
              V,
              " in ",
              assertion->getFunction()->getName());
    }
  }

  return;
}

static void update_accesses(llvm::Value &V, Type &type) {
  for (auto &use : V.uses()) {
    if (auto *access = into<AccessInst>(use.getUser())) {

      // We only need to update read/write since they are typed.
      auto *read = dyn_cast<ReadInst>(access);
      auto *write = dyn_cast<WriteInst>(access);
      if (not(read or write)) {
        continue;
      }

      println("MUTATE ", *access);

      // Fetch the type of the new access.
      auto *elem_type = &type;
      for (auto *index : access->indices()) {
        if (auto *collection_type = dyn_cast<CollectionType>(elem_type)) {
          elem_type = &collection_type->getElementType();

        } else if (auto *tuple_type = dyn_cast<TupleType>(elem_type)) {
          auto &field_const =
              MEMOIR_SANITIZE(dyn_cast<llvm::ConstantInt>(index),
                              "Index to tuple is not a constant!");
          auto field = field_const.getZExtValue();

          elem_type = &tuple_type->getFieldType(field);
        }
      }

      // Fetch the converted type instruction.
      auto &orig_func = access->getCalledFunction();
      auto &new_func =
          FunctionNames::convert_typed_function(orig_func, *elem_type);

      println("  NEW ", new_func.getName());

      // Update the called function.
      auto &call = access->getCallInst();

      call.setCalledFunction(&new_func);
    }
  }

  return;
}

struct NestedInfo {
  NestedInfo(llvm::Value &value, llvm::ArrayRef<unsigned> offsets)
    : _value(&value),
      _offsets(offsets) {}

  NestedInfo(llvm::Value &value) : NestedInfo(value, {}) {}

  llvm::Value &value() const {
    return *this->_value;
  }

  void value(llvm::Value &V) {
    this->_value = &V;
  }

  llvm::ArrayRef<unsigned> offsets() const {
    return this->_offsets;
  }

  Type &nested_type(Type &base_type) const {
    auto *type = &base_type;
    for (const auto offset : this->offsets()) {
      if (auto *collection_type = dyn_cast<CollectionType>(type)) {
        type = &collection_type->getElementType();
      } else if (auto *tuple_type = dyn_cast<TupleType>(type)) {
        type = &tuple_type->getFieldType(offset);
      }
    }

    return MEMOIR_SANITIZE(type, "Nested type is NULL!");
  }

  friend bool operator<(const NestedInfo &lhs, const NestedInfo &rhs) {
    if (lhs._value < rhs._value) {
      return true;
    }

    if (lhs.offsets().size() < rhs.offsets().size()) {
      return true;
    }

    auto lit = lhs.offsets().begin();
    auto rit = rhs.offsets().begin();
    auto lie = lhs.offsets().end();
    for (; lit != lie; ++lit, ++rit) {
      if (*lit < *rit) {
        return true;
      }
    }

    return false;
  }

protected:
  llvm::Value *_value;
  vector<unsigned> _offsets;
};

static void gather_nested_redefinitions(ordered_set<NestedInfo> &redefs,
                                        set<llvm::Value *> &visited,
                                        llvm::Value &V,
                                        llvm::ArrayRef<unsigned> offsets) {

  if (visited.count(&V) > 0) {
    return;
  } else {
    visited.insert(&V);
  }

  if (not offsets.empty()) {
    redefs.emplace(V, offsets);
  }

  for (auto &use : V.uses()) {
    auto *user = dyn_cast<llvm::Instruction>(use.getUser());

    if (auto *memoir = into<MemOIRInst>(user)) {
      if (auto *fold = dyn_cast<FoldInst>(memoir)) {
        if (fold->getObjectAsUse() == use) {

          // Fetch the element argument of the fold.
          // If it doesn't exist, then we have nothing to propagate.
          auto *arg = fold->getElementArgument();
          if (not arg) {
            continue;
          }

          // Get the type of the value so we can gather the correct offsets.
          auto *type = type_of(V);

          // Construct the nested offsets.
          vector<unsigned> nested_offsets(offsets.begin(), offsets.end());
          for (auto *index : fold->indices()) {
            if (auto *collection_type = dyn_cast<CollectionType>(type)) {

              nested_offsets.push_back(ELEMS);

              type = &collection_type->getElementType();

            } else if (auto *tuple_type = dyn_cast<TupleType>(type)) {
              // Unpack the field index.
              auto &field_constant =
                  MEMOIR_SANITIZE(dyn_cast<llvm::ConstantInt>(index),
                                  "Field index is non-constant!");
              auto field = field_constant.getZExtValue();

              nested_offsets.push_back(field);

              type = &tuple_type->getFieldType(field);
            }
          }

          nested_offsets.push_back(ELEMS);

          auto &collection_type =
              MEMOIR_SANITIZE(dyn_cast<CollectionType>(type),
                              "Fold over non-collection type!");

          auto &elem_type = collection_type.getElementType();

          if (not Type::is_primitive_type(elem_type)) {
            gather_nested_redefinitions(redefs, visited, *arg, nested_offsets);
          }
        }
      }
    } else if (auto *call = dyn_cast<llvm::CallBase>(user)) {
      auto *callee = call->getCalledFunction();
      MEMOIR_ASSERT(callee,
                    "Object passed into indirect call! ",
                    value_name(V),
                    " in ",
                    *call);

      auto operand_no = use.getOperandNo();
      MEMOIR_ASSERT(operand_no < callee->arg_size(),
                    "Object passed to argument out of range! ",
                    value_name(V),
                    " in ",
                    *call);

      auto &arg = MEMOIR_SANITIZE(callee->getArg(operand_no),
                                  "Argument ",
                                  operand_no,
                                  " in ",
                                  callee->getName(),
                                  " is NULL!");

      gather_nested_redefinitions(redefs, visited, arg, offsets);
    }
  }

  return;
}

static ordered_set<NestedInfo> gather_redefinitions(llvm::Value &V) {

  // Initialize the redefinitions.
  ordered_set<NestedInfo> redefs = {};

  // Gather base redefinitions.
  set<llvm::Value *> base_redefs = {};
  gather_base_redefinitions(base_redefs, V);

  // Gather nested redefinitions.
  set<llvm::Value *> visited = {};
  for (auto *base : base_redefs) {
    gather_nested_redefinitions(redefs, visited, *base, {});
  }

  // Insert the base redefinitions.
  for (auto *base : base_redefs) {
    redefs.emplace(*base, llvm::ArrayRef<unsigned>());
  }

  return redefs;
}

static ordered_set<NestedInfo> gather_redefinitions(MemOIRInst &I) {
  return gather_redefinitions(I.getCallInst());
}

#  if 0
static ordered_map<NestedInfo *, FoldInst *> gather_fold_arguments() {
  map<AllocInst *, Type *> types_to_mutate;
  for (auto *info : candidate) {
    auto *alloc = info->allocation;

    // Initialize the type to mutate if it doesnt exist already.
    if (types_to_mutate.count(alloc) == 0) {
      types_to_mutate[alloc] = &alloc->getType();
    }
    auto &type = *types_to_mutate[alloc];

    println("MERGE TYPE");
    println("  ", type);

    // Convert the type at the given offset to the size type.
    auto &module =
        MEMOIR_SANITIZE(alloc->getModule(),
                        "AllocInst does not belong to an LLVM module!");
    const auto &data_layout = module.getDataLayout();
    auto &size_type = Type::get_size_type(data_layout);

    auto &new_type = detail::convert_type(type, info->offsets, size_type);

    println("  ", new_type);

    types_to_mutate[alloc] = &new_type;
  }
}
#  endif

static void find_arguments(NestedInfo &info, vector<FoldInst *> &folds) {
  auto offsets = info.offsets();

  for (auto &use : info.value().uses()) {
    if (auto *fold = into<FoldInst>(use.getUser())) {
      if (use == fold->getObjectAsUse()) {

        // Try to match the indices.
        auto maybe_distance = fold->match_offsets(offsets);
        if (not maybe_distance) {
          // Mismatch.
        }

        auto distance = maybe_distance.value();

        if (distance == offsets.size()) {
          auto found = std::find(folds.begin(), folds.end(), fold);
          if (found == folds.end()) {
            folds.push_back(fold);
          }

        } else if (distance < offsets.size()) {
          // NOTE: this case may be unnecessary, the set of nested redefs will
          // already contain the argument.
          if (auto *elem_arg = fold->getElementArgument()) {
            NestedInfo nested(*elem_arg, offsets.drop_front(1 + distance));
            find_arguments(info, folds);
          }
        }
      }
    }
  }

  return;
}

#  if 0
static void update_fold_users(
    NestedInfo &info,
    map<NestedInfo *, vector<FoldInst *>> &folds_to_mutate,
    Type &type) {

  // For each of the folds, create a new function with the updated type.
  set<llvm::Function *> functions_to_delete = {};
  auto &to_mutate = folds_to_mutate[info];
  for (auto *fold : to_mutate) {

    // Fetch the index argument.
    auto &index_arg = fold->getIndexArgument();

    // Update the type of the function to match.
    auto *function = index_arg.getParent();
    auto *function_type = function->getFunctionType();

    bool found_real_use = false;
    if (function->hasNUsesOrMore(2)) {
      for (auto &use : function->uses()) {
        auto *user = use.getUser();
        if (not into<RetPHIInst>(*user)) {
          MEMOIR_ASSERT(not found_real_use, "Fold body has more than one use!");
          found_real_use = true;
        }
      }
    }

    // Rebuild the function type.
    vector<llvm::Type *> params(function_type->param_begin(),
                                function_type->param_end());
    params[index_arg.getArgNo()] = &llvm_size_type;

    auto *new_function_type =
        llvm::FunctionType::get(function_type->getReturnType(),
                                params,
                                function_type->isVarArg());

    // Create a new function with the new function type.
    auto *new_function = llvm::Function::Create(new_function_type,
                                                function->getLinkage(),
                                                function->getName(),
                                                this->M);

    // Clone the function with a changed parameter type.
    llvm::ValueToValueMapTy vmap;
    for (auto &old_arg : function->args()) {
      auto *new_arg = new_function->getArg(old_arg.getArgNo());
      vmap.insert({ &old_arg, new_arg });
    }
    llvm::SmallVector<llvm::ReturnInst *, 8> returns;
    llvm::CloneFunctionInto(new_function,
                            function,
                            vmap,
                            llvm::CloneFunctionChangeType::LocalChangesOnly,
                            returns);

    // Remove any pointer related attributes from the argument.
    auto attr_list = new_function->getAttributes();
    new_function->setAttributes(
        attr_list.removeParamAttributes(context, index_arg.getArgNo()));

    // Use the vmap to update any of the folds that we are patching.
    for (auto &[_, other_to_mutate] : folds_to_mutate) {
      for (auto &other_fold : other_to_mutate) {
        // Skip the fold we just updated.
        if (other_fold == fold) {
          continue;
        }

        // If the parent function is the same as the old function, find
        // this fold in the vmap.
        auto *parent_function = other_fold->getFunction();
        if (parent_function == function) {
          auto *new_inst = &*vmap[&other_fold->getCallInst()];
          auto *new_fold = into<FoldInst>(new_inst);

          // Update in-place.
          other_fold = new_fold;
        }
      }
    }

    // Remap any of the candidates that have been cloned.
    detail::update_candidates(std::next(candidates_it),
                              this->candidates.end(),
                              *function,
                              *new_function,
                              vmap);

    // Update the body of this fold.
    auto &body_use = fold->getBodyOperandAsUse();
    body_use.set(new_function);

    function->replaceAllUsesWith(new_function);

    new_function->takeName(function);

    function->deleteBody();

    functions_to_delete.insert(function);
  }

  for (auto *func : functions_to_delete) {
    func->eraseFromParent();
  }

  return;
}
#  endif

map<llvm::Value *, llvm::Value *> mutate_type(AllocInst &alloc, Type &type) {
  // Create a mapping for all variables that have been remapped.
  map<llvm::Value *, llvm::Value *> mapping = {};

  println("MUTATE");
  println("  ", alloc);
  println("  ", type);

  // Get the original type.
  auto &orig_type = alloc.getType();

  // Find all the paths that needs to be updated in the type.
  auto differences = type_differences(orig_type, type);

  // If there are no differences, then there is nothing to do!
  if (differences.empty()) {
    println("No differences found, continuing...");
    return mapping;
  }

  // Aggregate all of the differences that we have.
  bool type_differs = false;
  bool selection_differs = false;
  for (auto &diff : differences) {

    type_differs |= diff.type_differs();
    selection_differs |= diff.selection_differs();

    println(diff);
  }

  // Change the type operand of the allocation
  {
    MemOIRBuilder builder(alloc);

    auto *type_inst = builder.CreateTypeInst(type);

    alloc.getTypeOperandAsUse().set(&type_inst->getCallInst());
  }

  // Collect all nested redefinitions, merging the base redefinitions.
  auto redefs = gather_redefinitions(alloc);

  // TODO: handle collections passed as argument.

  // Update any nested type assertions.
  for (auto &info : redefs) {
    update_assertions(info.value(), info.nested_type(type));
  }

  // Update any type differences for accesses.
  if (type_differs) {
    for (auto &info : redefs) {
      update_accesses(info.value(), info.nested_type(type));
    }
  }

  // Update any type differences for function arguments.
  if (type_differs) {

    // Gather all fold arguments that need to be mutated.
    map<NestedInfo *, vector<>> for (auto &info : redefs) {
      auto args_to_mutate = find_arguments(info, type_differences);
    }

    // Update the arguments for fold bodies.
    for (auto &info : redefs) {
      update_fold_users(info, args_to_mutate, type);
    }
  }

  return mapping;
}

#endif

} // namespace llvm::memoir
