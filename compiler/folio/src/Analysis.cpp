#include "llvm/Support/CommandLine.h"

#include "folio/Benefit.hpp"
#include "folio/ProxyInsertion.hpp"
#include "folio/Utilities.hpp"

using namespace llvm::memoir;

namespace folio {

static llvm::cl::opt<bool> disable_proxy_propagation(
    "disable-proxy-propagation",
    llvm::cl::desc("Disable proxy propagation"),
    llvm::cl::init(false));

static llvm::cl::opt<bool> disable_proxy_sharing(
    "disable-proxy-sharing",
    llvm::cl::desc("Disable proxy sharing optimization"),
    llvm::cl::init(false));

void ProxyInsertion::gather_assoc_objects(Vector<ObjectInfo> &allocations,
                                          AllocInst &alloc,
                                          Type &type,
                                          Vector<unsigned> offsets) {

  if (auto *tuple_type = dyn_cast<TupleType>(&type)) {
    for (unsigned field = 0; field < tuple_type->getNumFields(); ++field) {

      auto new_offsets = offsets;
      new_offsets.push_back(field);

      this->gather_assoc_objects(allocations,
                                 alloc,
                                 tuple_type->getFieldType(field),
                                 new_offsets);
    }

  } else if (auto *collection_type = dyn_cast<CollectionType>(&type)) {
    auto &elem_type = collection_type->getElementType();

    // If this is an assoc, add the object information.
    if (isa<AssocType>(collection_type)) {
      allocations.push_back(ObjectInfo(alloc, offsets));
    }

    // Recurse on the element.
    auto new_offsets = offsets;
    new_offsets.push_back(-1);

    this->gather_assoc_objects(allocations, alloc, elem_type, new_offsets);
  }

  return;
}

static AllocInst *_find_base_object(llvm::Value &V,
                                    Vector<unsigned> &offsets,
                                    Set<llvm::Value *> &visited) {
  if (visited.count(&V) > 0) {
    return nullptr;
  } else {
    visited.insert(&V);
  }

  if (auto *arg = dyn_cast<llvm::Argument>(&V)) {
    auto &func =
        MEMOIR_SANITIZE(arg->getParent(), "Argument has no parent function");
    if (auto *fold = FoldInst::get_single_fold(func)) {
      if (arg == fold->getElementArgument()) {
        auto it = offsets.begin();
        for (auto *index : fold->indices()) {
          unsigned offset = -1;
          if (auto *index_const = dyn_cast<llvm::ConstantInt>(index)) {
            offset = index_const->getZExtValue();
          }
          it = offsets.insert(it, offset);
        }
        offsets.insert(it, -1);

        return _find_base_object(fold->getObject(), offsets, visited);
      } else if (auto *operand = fold->getOperandForArgument(*arg)) {
        return _find_base_object(*operand->get(), offsets, visited);
      }
    }

  } else if (auto *phi = dyn_cast<llvm::PHINode>(&V)) {
    for (auto &incoming : phi->incoming_values()) {
      auto *base = _find_base_object(*incoming.get(), offsets, visited);
      if (base) {
        return base;
      }
    }
  } else if (auto *alloc = into<AllocInst>(&V)) {
    return alloc;

  } else if (auto *update = into<UpdateInst>(&V)) {
    return _find_base_object(update->getObject(), offsets, visited);

  } else if (auto *ret_phi = into<RetPHIInst>(&V)) {
    return _find_base_object(ret_phi->getInput(), offsets, visited);

  } else if (auto *call = dyn_cast<llvm::CallBase>(&V)) {
    // TODO
    warnln("Base object returned from call!");
  }

  return nullptr;
}

ObjectInfo *ProxyInsertion::find_base_object(llvm::Value &V,
                                             AccessInst &access) {

  infoln("FINDING BASE OF ", access);

  auto *type = type_of(V);

  Vector<unsigned> offsets = {};
  for (auto *index : access.indices()) {
    if (auto *tuple_type = dyn_cast<TupleType>(type)) {
      auto index_const = dyn_cast<llvm::ConstantInt>(index);
      auto field = index_const->getZExtValue();
      type = &tuple_type->getFieldType(field);

      offsets.push_back(field);

    } else if (auto *collection_type = dyn_cast<CollectionType>(type)) {
      type = &collection_type->getElementType();

      offsets.push_back(-1);
    }
  }
  if (isa<FoldInst>(&access)) {
    offsets.push_back(-1);
  }

  Set<llvm::Value *> visited = {};

  auto *alloc = _find_base_object(V, offsets, visited);

  if (not alloc) {
    return nullptr;
  }

  this->propagators.emplace_back(*alloc, offsets);
  auto &info = this->propagators.back();

  infoln("FOUND PROPAGATOR ", info);

  return &info;
}

static bool object_can_share(Type &type,
                             llvm::Function *func,
                             ObjectInfo &other) {
  // Check that the key types match.
  auto *other_alloc = other.allocation;
  auto &other_type = MEMOIR_SANITIZE(dyn_cast<AssocType>(&other.get_type()),
                                     "Non-assoc type, unhandled.");

  if (&type != &other_type.getKeyType()) {
    return false;
  }

  // Check that they share a parent function.
  // NOTE: this is overly conservative
  auto *other_func = other.allocation->getFunction();
  if (func != other_func) {
    return false;
  }

  return true;
}

static bool propagator_can_share(Type &type,
                                 llvm::Function *func,
                                 ObjectInfo &other) {
  auto *other_alloc = other.allocation;
  auto &other_type = other.get_type();

  if (&type != &other_type) {
    return false;
  }

  // Check that they share a parent basic block.
  auto *other_func = other_alloc->getFunction();
  if (func != other_func) {
    return false;
  }

  return true;
}

static void share_proxies(Vector<ObjectInfo> &objects,
                          Vector<ObjectInfo> &propagators,
                          Vector<Candidate> &candidates) {

  Set<const ObjectInfo *> used = {};
  for (auto it = objects.begin(); it != objects.end(); ++it) {
    auto &info = *it;

    if (used.count(&info)) {
      continue;
    }

    auto *alloc = info.allocation;
    auto *bb = alloc->getParent();
    auto *func = bb->getParent();

    auto &type = MEMOIR_SANITIZE(dyn_cast<AssocType>(&info.get_type()),
                                 "Non-assoc type, unhandled.");
    auto &key_type = type.getKeyType();

    candidates.emplace_back();
    auto &candidate = candidates.back();
    candidate.push_back(&info);

    // Compute the benefit of the candidate as is.
    candidate.benefit = benefit(candidate);

    // Find all other allocations in the same function as this one.
    if (not disable_proxy_sharing) {

      // Collect the set of objects that can share, and their solo benefit.
      Map<ObjectInfo *, int> shareable = {};

      for (auto &other : objects) {
        if (used.contains(&other) or &other == &info) {
          continue;
        }

        if (object_can_share(key_type, func, other)) {
          shareable[&other] = benefit({ &other });
        }
      }

      // Find all propagators in the same function as this one.
      for (auto &other : propagators) {
        if (used.contains(&other) or &other == &info) {
          continue;
        }

        if (propagator_can_share(key_type, func, other)) {
          shareable[&other] = benefit({ &other });
        }
      }

      infoln("SHAREABLE");
      for (const auto &[other, _] : shareable) {
        infoln("  ", *other);
      }
      infoln();

      // Iterate until we can't find a new object to add to the candidate.
      bool fresh;
      do {
        fresh = false;

        infoln("CURRENT ", candidate);
        infoln("  BENEFIT ", candidate.benefit);

        for (auto jt = shareable.begin(); jt != shareable.end();) {
          const auto &[other, single_benefit] = *jt;

          infoln("  WHAT IF? ", *other);

          // Compute the benefit of adding this candidate.
          candidate.push_back(other);
          auto new_benefit = benefit(candidate);
          infoln("    NEW BENEFIT ", new_benefit);
          infoln("    SUM BENEFIT ", candidate.benefit + single_benefit);
          infoln();

          if (new_benefit > (candidate.benefit + single_benefit)) {
            fresh = true;
            candidate.benefit = new_benefit;

            // Erase the object from the search list.
            jt = shareable.erase(jt);

          } else {
            // If there's no benefit, roll it back.
            candidate.pop_back();

            // Continue iterating.
            ++jt;
          }
        }

      } while (fresh);
    }

    // If there is no benefit, roll back the candidate.
    if (candidate.size() == 0 or candidate.benefit == 0) {
      candidates.pop_back();

    } else {
      // Mark the objects in the candidate as being used.
      used.insert(candidate.begin(), candidate.end());
    }
  }

  for (auto it = candidates.begin(); it != candidates.end(); ++it) {
    auto &candidate = *it;

    // Find any candidates that share an allocation with this one.
    Set<AllocInst *> allocations = {};
    for (auto *info : candidate) {
      allocations.insert(info->allocation);
    }

    auto benefit = candidate.benefit;

    bool other_wins = false;
    for (auto jt = std::next(it); jt != candidates.end();) {
      auto &other = *jt;

      bool aliases = false;
      for (auto *info : other) {
        if (allocations.contains(info->allocation)) {
          aliases = true;
        }
      }

      if (aliases) {
        // Which candidate needs to go?
        if (other.benefit > benefit) {
          // Other candidate wins, delete this candidate.
          other_wins = true;
          break;
        } else {
          // This candidate wins, delete other candidate.
          jt = candidates.erase(jt);
        }
      } else {
        // If we don't alias, then its all hunky dory.
        ++jt;
      }
    }

    if (other_wins) {
      it = candidates.erase(it);
    }
  }

  for (auto &candidate : candidates) {
    println("CANDIDATE:");
    println("  BENEFIT=", candidate.benefit);
    for (const auto *info : candidate) {
      println("  ", *info);
    }
  }
}

static void gather_nested_propagators(Vector<ObjectInfo> &propagators,
                                      const Set<Type *> &types,
                                      AllocInst &alloc,
                                      Type &type,
                                      llvm::ArrayRef<unsigned> offsets = {}) {

  if (auto *collection_type = dyn_cast<CollectionType>(&type)) {

    auto &elem_type = collection_type->getElementType();

    Vector<unsigned> nested_offsets(offsets.begin(), offsets.end());

    // Recurse on the element.
    nested_offsets.push_back(unsigned(-1));
    gather_nested_propagators(propagators,
                              types,
                              alloc,
                              elem_type,
                              nested_offsets);

  } else if (auto *tuple_type = dyn_cast<TupleType>(&type)) {

    Vector<unsigned> nested_offsets(offsets.begin(), offsets.end());

    for (auto field = 0; field < tuple_type->getNumFields(); ++field) {
      auto &field_type = tuple_type->getFieldType(field);

      // Recurse on the field.
      nested_offsets.push_back(field);
      gather_nested_propagators(propagators,
                                types,
                                alloc,
                                field_type,
                                nested_offsets);
      nested_offsets.pop_back();
    }

  } else {
    if (types.contains(&type)) {
      propagators.emplace_back(alloc, offsets);
    }
  }
}

static void gather_propagators(Vector<ObjectInfo> &propagators,
                               const Set<Type *> &types,
                               llvm::Module &M) {
  // Fetch all allocations.
  auto *alloc_func =
      FunctionNames::get_memoir_function(M, MemOIR_Func::ALLOCATE);

  for (auto &use : alloc_func->uses()) {
    auto *alloc = into<AllocInst>(use.getUser());
    if (not alloc) {
      continue;
    }

    println("ALLOC ", *alloc);

    // Recurse into the type to find relevant objects.
    gather_nested_propagators(propagators, types, *alloc, alloc->getType());
  }

  for (auto &prop : propagators) {
    prop.analyze();
  }
}

void ProxyInsertion::analyze() {
  auto &M = this->M;

  // Gather all possible allocations.
  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (auto *alloc = into<AllocInst>(&I)) {
          // Gather all of the Assoc allocations.
          this->gather_assoc_objects(this->objects,
                                     *alloc,
                                     alloc->getType(),
                                     {});
        }
      }
    }
  }

  println();
  println("FOUND OBJECTS ", this->objects.size());
  for (auto &info : this->objects) {
    println("  ", info);
  }
  println();

  // Analyze each of the objects.
  for (auto &info : this->objects) {
    info.analyze();

    forward_analysis(info.encoded);
  }

  // With the set of values that need to be encoded/decoded, we will find
  // collections that can be used to propagate proxied values.
  if (not disable_proxy_propagation) {

    // Find all objects whose value type is the same as any of the objects we've
    // discovered.
    Set<Type *> types = {};
    for (auto &info : this->objects) {
      if (auto *assoc_type = dyn_cast<AssocType>(&info.get_type())) {
        auto &key_type = assoc_type->getKeyType();
        types.insert(&key_type);
      }
    }

    // Gather all objects in the program that have elements of a relevant type.
    gather_propagators(this->propagators, types, M);

#if 0
    // From the use information, find any collection elements that _could_
    // propagate the proxy.
    Map<llvm::Function *, Set<llvm::Value *>> encoded = {};
    Map<llvm::Function *, Set<llvm::Use *>> to_encode = {};
    for (auto &info : this->objects) {
      for (const auto &[func, locals] : info.encoded) {
        encoded[func].insert(locals.begin(), locals.end());
      }
      for (const auto &uses : { info.to_encode, info.to_addkey }) {
        for (const auto &[func, locals] : uses) {
          to_encode[func].insert(locals.begin(), locals.end());
        }
      }
    }

    // Gather the propagators from the encoded values.
    this->gather_propagators(encoded, to_encode);
#endif

    println();
    println("FOUND PROPAGATORS ", this->propagators.size());
    for (auto &info : this->propagators) {
      println("  ", info);
    }
    println();
  }

  // Use a heuristic to share proxies between collections.
  share_proxies(this->objects, this->propagators, this->candidates);
}

} // namespace folio
