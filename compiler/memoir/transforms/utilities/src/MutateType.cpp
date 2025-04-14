#include "llvm/ADT/ArrayRef.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "memoir/ir/Builder.hpp"
#include "memoir/transforms/utilities/MutateType.hpp"
#include "memoir/utility/FunctionNames.hpp"

namespace llvm::memoir {

// We will use two special offset values to represent either the keys or
// elements at a given offset.
#define ELEMS unsigned(-1)

enum DifferenceKind : uint8_t {
  NoDifference = 0,
  TypeDiffers = 1 << 0,
  CollectionDiffers = 1 << 1,
  SelectionDiffers = 1 << 2,
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

  bool collection_differs() const {
    return (this->_kind & DifferenceKind::CollectionDiffers)
           != DifferenceKind::NoDifference;
  }

  bool selection_differs() const {
    return (this->_kind & DifferenceKind::SelectionDiffers)
           != DifferenceKind::NoDifference;
  }

  llvm::ArrayRef<unsigned> offsets() const {
    return this->_offsets;
  }

  Type &convert_type(Type &new_type) const {
    auto *type = &new_type;

    for (auto offset : this->offsets()) {
      if (auto *collection = dyn_cast<CollectionType>(type)) {
        type = &collection->getElementType();
      } else if (auto *tuple = dyn_cast<TupleType>(type)) {
        type = &tuple->getFieldType(offset);
      }
    }

    return MEMOIR_SANITIZE(type, "Converted type is NULL!");
  }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const Difference &diff) {
    os << "DIFFERS:";
    if (diff.type_differs()) {
      os << " type";
    }
    if (diff.collection_differs()) {
      os << " collection";
    }
    if (diff.selection_differs()) {
      os << " selection";
    }
    os << "\n";

    os << "  ";
    for (auto offset : diff.offsets()) {
      switch (offset) {
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

  llvm::ArrayRef<Difference> diffs() const {
    return *this;
  }

  void add(DifferenceKind kind, llvm::ArrayRef<unsigned> offsets) {
    this->emplace_back(kind, offsets);
  }

  const Difference *find(llvm::ArrayRef<unsigned> offsets) {
    for (const auto &diff : this->diffs()) {

      bool matches = true;
      auto offset_it = offsets.begin(), offset_ie = offsets.end();
      auto diff_it = diff.offsets().begin(), diff_ie = diff.offsets().end();

      for (; offset_it != offset_ie and diff_it != diff_ie;
           ++offset_it, ++diff_it) {
        if (*offset_it != *diff_it) {
          matches = false;
          break;
        }
      }

      if (matches and offset_it == offset_ie and diff_it == diff_ie) {
        return &diff;
      }
    }

    return nullptr;
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

      type_differences(differences, offsets, base_key, other_key);

      // Check differences on the element.
      auto &base_elem = base_assoc->getElementType();
      auto &other_elem = other_assoc->getElementType();

      nested_offsets.push_back(ELEMS);
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

      nested_offsets.push_back(ELEMS);
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
  if (auto *base_assoc = dyn_cast<AssocType>(&base)) {
    if (auto *other_seq = dyn_cast<SequenceType>(&other)) {
      auto &base_elem = base_assoc->getElementType();
      auto &other_elem = other_seq->getElementType();

      vector<unsigned> nested_offsets(offsets.begin(), offsets.end());

      nested_offsets.push_back(ELEMS);
      type_differences(differences, nested_offsets, base_elem, other_elem);
      nested_offsets.pop_back();

      differences.emplace_back(DifferenceKind::CollectionDiffers, offsets);

      return;
    }
  }

  // Or from sequence -> assoc

  // Otherwise, the mutation is not isomorphic, unhandled!
  // Do it yourself!
  MEMOIR_UNREACHABLE("Non-isomorphic type mutation! ", base, " => ", other);

  return;
}

static Differences type_differences(Type &base, Type &other) {
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
        if (&use == &update->getObjectAsUse()) {
          gather_base_redefinitions(redefs, update->getResult());
        }

      } else if (auto *fold = dyn_cast<FoldInst>(memoir)) {
        if (&use == &fold->getInitialAsUse()) {
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
        if (&fold->getObjectAsUse() == &use) {

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
        } else if (auto *closed_arg = fold->getClosedArgument(use)) {
          gather_nested_redefinitions(redefs, visited, *closed_arg, offsets);
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

static void gather_offsets(vector<unsigned> &offsets, AccessInst &access) {

  auto *type = &access.getObjectType();

  for (auto *index : access.indices()) {
    if (auto *tuple = dyn_cast<TupleType>(type)) {
      auto &index_const = MEMOIR_SANITIZE(dyn_cast<llvm::ConstantInt>(index),
                                          "Field index is not a constant!");
      auto field = index_const.getZExtValue();
      offsets.push_back(field);

    } else if (auto *collection = dyn_cast<CollectionType>(type)) {
      offsets.push_back(ELEMS);
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

static void update_assertions(llvm::Value &V, Type &type) {
  for (auto &use : V.uses()) {
    if (auto *assertion = into<AssertTypeInst>(use.getUser())) {
      MemOIRBuilder builder(*assertion);

      auto *type_value = &builder.CreateTypeInst(type)->getCallInst();

      assertion->getTypeOperandAsUse().set(type_value);
    }
  }

  return;
}

static void update_accesses(llvm::Value &V, Type &type) {
  for (auto &use : V.uses()) {
    if (auto *access = into<AccessInst>(use.getUser())) {

      // Handle read/write accesses together.
      auto *read = dyn_cast<ReadInst>(access);
      auto *write = dyn_cast<WriteInst>(access);
      if (not(read or write)) {
        continue;
      }

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

      // Update the called function.
      auto &call = access->getCallInst();

      call.setCalledFunction(&new_func);
    }
  }

  return;
}

static void update_has(const NestedInfo &info,
                       llvm::ArrayRef<unsigned> diff_offsets) {
  auto &value = info.value();

  set<llvm::Instruction *> to_cleanup = {};

  for (auto &use : value.uses()) {
    auto *user = dyn_cast<llvm::Instruction>(use.getUser());
    if (not user) {
      continue;
    }

    if (auto *has = into<HasInst>(user)) {

      println(*has);

      // Check that the operation is at the correct offset.
      if (info.offsets().size() != diff_offsets.size()) {
        continue;
      }

      bool matches = true;
      auto remaining_offsets = diff_offsets;
      for (auto offset : info.offsets()) {
        if (offset == remaining_offsets.front()) {
          matches = false;
          break;
        }
        remaining_offsets = remaining_offsets.drop_front();
      }

      if (not matches) {
        continue;
      }

      // Check if the indices match the difference offset.
      // If they don't, SKIP.
      auto distance = has->match_offsets(remaining_offsets);
      if (not distance and distance.value() < remaining_offsets.size()) {
        continue;
      }

      // REWRITE has(c, k) => k < size(c)
      MemOIRBuilder builder(*has);

      auto &object = has->getObject();
      vector<llvm::Value *> indices(has->indices_begin(), has->indices_end());
      auto *last_index = indices.back();
      indices.pop_back();

      auto *size =
          &builder.CreateSizeInst(&object, indices, "has.")->getCallInst();

      auto *cmp = builder.CreateICmpULT(last_index, size, "has.");

      has->asValue().replaceAllUsesWith(cmp);

      to_cleanup.insert(user);
    }
  }

  for (auto *inst : to_cleanup) {
    inst->removeFromParent();
    inst->dropAllReferences();
  }
}

static void find_arguments(map<llvm::Argument *, Type *> &args_to_mutate,
                           const NestedInfo &info,
                           Differences &diffs,
                           Type &type) {

  auto &nested_type = info.nested_type(type);

  // Find fold operations on this collection.
  for (auto &use : info.value().uses()) {
    if (auto *fold = into<FoldInst>(use.getUser())) {
      if (&use == &fold->getObjectAsUse()) {

        // Gather the offsets for the fold.
        vector<unsigned> offsets(info.offsets().begin(), info.offsets().end());
        gather_offsets(offsets, *fold);

        // Check for key type differences.
        auto *key_diff = diffs.find(offsets);
        if (key_diff and key_diff->type_differs()) {
          auto &key_arg = fold->getIndexArgument();

          Type *converted_key_type = nullptr;
          auto &converted_type = key_diff->convert_type(nested_type);
          if (auto *assoc_type = dyn_cast<AssocType>(&converted_type)) {
            converted_key_type = &assoc_type->getKeyType();
          } else if (auto *seq_type = dyn_cast<SequenceType>(&converted_type)) {
            auto &module = MEMOIR_SANITIZE(fold->getModule(),
                                           "FoldInst has no parent module!");
            converted_key_type = &Type::get_size_type(module.getDataLayout());
          } else {
            println("UNHANDLED PARENT TYPE ", converted_type);
          }

          println("KEY  DIFF: ",
                  key_arg,
                  " : ",
                  converted_key_type,
                  " IN ",
                  fold->getBody().getName());
          args_to_mutate[&key_arg] = converted_key_type;
        }

        // Check for value type differences.
        if (auto *elem_arg = fold->getElementArgument()) {
          offsets.push_back(ELEMS);
          auto *elem_diff = diffs.find(offsets);
          if (elem_diff and elem_diff->type_differs()) {
            auto &converted_type = elem_diff->convert_type(nested_type);
            println("ELEM DIFF: ",
                    *elem_arg,
                    " : ",
                    converted_type,
                    " IN ",
                    fold->getBody().getName());
            args_to_mutate[elem_arg] = &converted_type;
          }
        }
      }
    }
  }

  return;
}

static void update_arguments(ordered_set<NestedInfo> &redefs,
                             Differences &diffs,
                             Type &type,
                             OnFuncClone on_func_clone) {

  map<llvm::Argument *, Type *> to_mutate = {};

  // Find all arguments to mutate.
  for (auto &info : redefs) {
    find_arguments(to_mutate, info, diffs, type);
  }

  // Gather the set of functions to update.
  map<llvm::Function *, vector<llvm::Argument *>> functions = {};
  for (const auto &[arg, _type] : to_mutate) {
    functions[arg->getParent()].push_back(arg);
  }

  // Create a clone of each function with the new argument types.
  set<llvm::Function *> functions_to_delete = {};
  for (const auto &[func, args] : functions) {
    auto &context = func->getContext();
    auto &module = MEMOIR_SANITIZE(func->getParent(),
                                   "Function has no parent module: ",
                                   func->getName());

    auto *func_type = func->getFunctionType();

    // Rebuild the function type.
    vector<llvm::Type *> params(func_type->param_begin(),
                                func_type->param_end());

    for (auto *arg : args) {
      auto arg_no = arg->getArgNo();
      if (arg_no < params.size()) {
        auto *arg_type = to_mutate[arg];
        if (not arg_type) {
          println(*arg,
                  " in ",
                  func->getName(),
                  " is missing type information!");
        }
        auto *arg_llvm_type = arg_type->get_llvm_type(context);
        params[arg_no] = arg_llvm_type;
      } else {
        MEMOIR_ASSERT(func_type->isVarArg(),
                      "Arg ",
                      arg_no,
                      " is out of bounds in non-variadic function ",
                      func->getName());
      }
    }

    auto *new_func_type = llvm::FunctionType::get(func_type->getReturnType(),
                                                  params,
                                                  func_type->isVarArg());

    // Create a new function with the new function type.
    auto *new_func = llvm::Function::Create(new_func_type,
                                            func->getLinkage(),
                                            func->getName(),
                                            module);

    // Clone the function with a changed parameter type.
    llvm::ValueToValueMapTy vmap;
    for (auto &old_arg : func->args()) {
      auto *new_arg = new_func->getArg(old_arg.getArgNo());
      vmap.insert({ &old_arg, new_arg });
    }
    llvm::SmallVector<llvm::ReturnInst *, 8> returns;
    llvm::CloneFunctionInto(new_func,
                            func,
                            vmap,
                            llvm::CloneFunctionChangeType::LocalChangesOnly,
                            returns);

    // Invoke the callback.
    on_func_clone(*func, *new_func, vmap);

    // Remove any pointer related attributes from the updated arguments.
    auto attr_list = new_func->getAttributes();
    for (auto *arg : args) {
      attr_list = attr_list.removeParamAttributes(context, arg->getArgNo());
    }
    new_func->setAttributes(attr_list);

    func->replaceAllUsesWith(new_func);

    new_func->takeName(func);

    func->deleteBody();

    functions_to_delete.insert(func);
  }

  for (auto *func : functions_to_delete) {
    func->eraseFromParent();
  }

  return;
}

void mutate_type(AllocInst &alloc, Type &type, OnFuncClone on_func_clone) {

  debugln("MUTATE");
  debugln("  ", alloc, " in ", alloc.getFunction()->getName());
  debugln("  ", type);

  // Get the original type.
  auto &orig_type = alloc.getType();

  // Find all the paths that needs to be updated in the type.
  auto differences = type_differences(orig_type, type);

  // If there are no differences, then there is nothing to do!
  if (differences.empty()) {
    debugln("No differences found, continuing...");
    return;
  }

  // Aggregate all of the differences that we have.
  bool type_differs = false;
  bool collection_differs = false;
  bool selection_differs = false;
  for (auto &diff : differences) {

    type_differs |= diff.type_differs();
    collection_differs |= diff.collection_differs();
    selection_differs |= diff.selection_differs();

    debugln(diff);
  }

  // Change the type operand of the allocation
  {
    debugln("Updating allocation type...");

    MemOIRBuilder builder(alloc);

    auto *type_inst = builder.CreateTypeInst(type);

    alloc.getTypeOperandAsUse().set(&type_inst->getCallInst());

    debugln("Done updating allocation type...");
  }

  // Collect all nested redefinitions, merging the base redefinitions.
  debugln("Gathering redefinitions...");
  auto redefs = gather_redefinitions(alloc);
  debugln("Done gathering redefinitions.");

  // TODO: handle collections passed as argument.

  // Update any nested type assertions.
  debugln("Updating assertions...");
  for (auto &info : redefs) {
    update_assertions(info.value(), info.nested_type(type));
  }
  debugln("Done updating assertions.");

  // Update any type differences for accesses.
  debugln("Updating accesses...");
  if (type_differs) {
    for (auto &info : redefs) {
      update_accesses(info.value(), info.nested_type(type));
    }
    debugln("Done updating accesses.");
  } else {
    debugln("No accesses to update.");
  }

  if (collection_differs) {

    // Update the allocation to take a size.

    for (auto &diff : differences) {
      if (diff.collection_differs()) {
        // Update any has operations to be bounds checks.
        for (auto &info : redefs) {
          update_has(info, diff.offsets());
        }
      }
    }
  }

  // Update any type differences for function arguments.
  debugln("Updating arguments...");
  if (type_differs) {
    update_arguments(redefs, differences, type, on_func_clone);
    debugln("Done updating arguments.");
  } else {
    debugln("No arguments to update.");
  }

  return;
}

} // namespace llvm::memoir
