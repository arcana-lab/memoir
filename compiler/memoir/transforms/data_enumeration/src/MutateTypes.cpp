#include "memoir/support/AssocList.hpp"
#include "memoir/support/DataTypes.hpp"
#include "memoir/support/Print.hpp"
#include "memoir/transforms/utilities/MutateType.hpp"

#include "DataEnumeration.hpp"
#include "Utilities.hpp"

using namespace memoir;

namespace memoir {

// HELPER types.
using ParamTypes =
    OrderedMultiMap<llvm::Function *, Pair<llvm::Argument *, llvm::Type *>>;

using AllocTypes = AssocList<AllocInst *, Type *>;

// Helper functions.
static Type &convert_to_sequence_type(Type &base,
                                      llvm::ArrayRef<unsigned> offsets) {

  infoln("CONVERT TO SEQUENCE:");
  infoln(base);
  for (auto offset : offsets) {
    info(offset, ", ");
  }
  infoln();

  if (auto *tuple_type = dyn_cast<TupleType>(&base)) {

    Vector<Type *> fields = tuple_type->fields();

    auto field = offsets[0];

    fields[field] = &convert_to_sequence_type(tuple_type->getFieldType(field),
                                              offsets.drop_front());

    return TupleType::get(fields);

  } else if (auto *seq_type = dyn_cast<SequenceType>(&base)) {

    return SequenceType::get(
        convert_to_sequence_type(seq_type->getElementType(),
                                 offsets.drop_front()),
        seq_type->get_selection());

  } else if (auto *assoc_type = dyn_cast<AssocType>(&base)) {

    // If the offsets are empty, replace the keys.
    if (offsets.empty()) {
      auto &converted = SequenceType::get(assoc_type->getValueType());
      infoln("CONVERTED: ", converted);
      return converted;
    }

    return AssocType::get(assoc_type->getKeyType(),
                          convert_to_sequence_type(assoc_type->getValueType(),
                                                   offsets.drop_front()),
                          assoc_type->get_selection());

  } else if (offsets.empty()) {
    return base;
  }

  MEMOIR_UNREACHABLE("Failed to convert type!");
}

static Type &convert_element_type(Type &base,
                                  llvm::ArrayRef<unsigned> offsets,
                                  Type &new_type,
                                  bool is_nested = false) {

  if (auto *tuple_type = dyn_cast<TupleType>(&base)) {

    Vector<Type *> fields = tuple_type->fields();

    auto field = offsets[0];

    fields[field] = &convert_element_type(tuple_type->getFieldType(field),
                                          offsets.drop_front(),
                                          new_type,
                                          is_nested);

    return TupleType::get(fields);

  } else if (auto *seq_type = dyn_cast<SequenceType>(&base)) {

    return SequenceType::get(convert_element_type(seq_type->getElementType(),
                                                  offsets.drop_front(),
                                                  new_type,
                                                  true),
                             seq_type->get_selection());

  } else if (auto *assoc_type = dyn_cast<AssocType>(&base)) {

    // If the offsets are empty, replace the keys.
    if (offsets.empty()) {

      auto selection =
          DataEnumeration::get_enumerated_impl(*assoc_type, is_nested);

      return AssocType::get(new_type, assoc_type->getValueType(), selection);
    }

    return AssocType::get(assoc_type->getKeyType(),
                          convert_element_type(assoc_type->getValueType(),
                                               offsets.drop_front(),
                                               new_type,
                                               true),
                          assoc_type->get_selection());

  } else if (offsets.empty()) {
    return new_type;
  }

  MEMOIR_UNREACHABLE("Failed to convert type!");
}

static void collect_types_to_mutate(llvm::Module &module,
                                    llvm::ArrayRef<ObjectInfo *> objects,
                                    const DataEnumeration::TransformInfo &info,
                                    ParamTypes &params_to_mutate,
                                    AllocTypes &allocs_to_mutate) {

  // Fetch relevant type information.
  auto &data_layout = module.getDataLayout();
  auto &context = module.getContext();

  auto &size_type = Type::get_size_type(data_layout);
  auto &llvm_size_type = *size_type.get_llvm_type(context);

  // Collect any function parameters whose type has changed because the
  // argument propagates an encoded value.
  for (const auto &[func, values] : info.encoded)
    for (auto *val : values)
      if (auto *arg = dyn_cast<llvm::Argument>(val))
        if (arg->getType() != &llvm_size_type)
          params_to_mutate.emplace(arg->getParent(),
                                   make_pair(arg, &llvm_size_type));

  // Collect the types that each candidate needs to be mutated to.
  for (auto *obj : objects) {
    auto *base = dyn_cast<BaseObjectInfo>(obj);
    if (not base)
      continue;
    auto *alloc = &base->allocation();

    // Initialize the type to mutate if it doesnt exist already.
    auto found = allocs_to_mutate.find(alloc);
    if (found == allocs_to_mutate.end()) {
      allocs_to_mutate[alloc] = &alloc->getType();
    }
    auto &type = allocs_to_mutate[alloc];

#if 0
    // If the object is a total proxy, update it to be a sequence.
    if (is_total_proxy(*info, candidate.to_addkey)
        and not disable_total_proxy) {
      infoln(Style::BOLD, Colors::GREEN, "FOUND TOTAL PROXY", Style::RESET);
      type = &convert_to_sequence_type(*type, info->offsets());

    } else
#endif
    // Convert the type at the given offset to the size type.
    type = &convert_element_type(*type, obj->offsets(), size_type);
  }
}

static void mutate_found_types(ParamTypes &params_to_mutate,
                               AllocTypes &allocs_to_mutate) {

  Set<llvm::Function *> to_cleanup = {};
  for (auto it = params_to_mutate.begin(); it != params_to_mutate.end();) {
    auto *func = it->first;

    auto *module = func->getParent();
    auto &data_layout = module->getDataLayout();

    auto *func_type = func->getFunctionType();

    // Collect the original parameter types.
    Vector<llvm::Type *> param_types(func_type->param_begin(),
                                     func_type->param_end());

    // Update the parameter types that are encoded.
    debugln("MUTATE PARAMS OF ", func->getName());
    for (; it != params_to_mutate.upper_bound(func); ++it) {
      auto [arg, type] = it->second;
      auto arg_idx = arg->getArgNo();

      param_types[arg_idx] = type;

      debugln("  ARG ", *arg, " TO ", *type);
    }

    // Create the new function type.
    auto *new_func_type = llvm::FunctionType::get(func_type->getReturnType(),
                                                  param_types,
                                                  func_type->isVarArg());

    // TODO: Fix the attributes on the new function.

    // Create the empty function to clone into.
    auto &new_func = MEMOIR_SANITIZE(llvm::Function::Create(new_func_type,
                                                            func->getLinkage(),
                                                            func->getName(),
                                                            module),
                                     "Failed to create new function.");

    debugln("CLONING ", func->getName());

    // Update the function to the new type.
    llvm::ValueToValueMapTy vmap;
    for (auto &old_arg : func->args()) {
      auto *new_arg = new_func.getArg(old_arg.getArgNo());
      vmap.insert({ &old_arg, new_arg });
    }
    llvm::SmallVector<llvm::ReturnInst *, 8> returns;
    llvm::CloneFunctionInto(&new_func,
                            func,
                            vmap,
                            llvm::CloneFunctionChangeType::LocalChangesOnly,
                            returns);

    new_func.takeName(func);
    func->replaceAllUsesWith(&new_func);

    // Clean up the old function.
    to_cleanup.insert(func);
  }

  // Mutate the types in the program.
  auto mutate_it = allocs_to_mutate.begin(), mutate_ie = allocs_to_mutate.end();
  for (; mutate_it != mutate_ie; ++mutate_it) {
    auto *alloc = mutate_it->first;
    auto *type = mutate_it->second;

    // Mutate the type.
    mutate_type(*alloc,
                *type,
                [&](llvm::Function &old_func,
                    llvm::Function &new_func,
                    llvm::ValueToValueMapTy &vmap) {
                  // Update allocations marked for mutation.
                  for (auto it = std::next(mutate_it); it != mutate_ie; ++it) {
                    auto *alloc = it->first;
                    auto &inst = alloc->getCallInst();

                    if (inst.getFunction() == &old_func) {
                      auto *new_inst = &*vmap[&inst];
                      auto *new_alloc = into<AllocInst>(new_inst);

                      // Update in-place.
                      it->first = new_alloc;
                    }
                  }
                });
  }

  for (auto *func : to_cleanup) {
    func->deleteBody();
    func->eraseFromParent();
  }
}

// Entrypoint.
void DataEnumeration::mutate_types() {
  // Find parameter and allocation types that need to be mutated.
  debugln("=== FIND NEW PARAM TYPES ===");
  ParamTypes params_to_mutate = {};
  AssocList<AllocInst *, Type *> allocs_to_mutate;
  for (const auto &[parent, objects] : this->equiv) {
    collect_types_to_mutate(module,
                            objects,
                            this->to_transform.at(parent),
                            params_to_mutate,
                            allocs_to_mutate);
  }
  debugln();

  // Mutate the types that we found.
  mutate_found_types(params_to_mutate, allocs_to_mutate);
}

} // namespace memoir
