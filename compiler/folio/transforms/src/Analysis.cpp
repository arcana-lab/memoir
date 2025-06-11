#include "llvm/Support/CommandLine.h"

#include "folio/transforms/ProxyInsertion.hpp"
#include "folio/transforms/Utilities.hpp"

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

void ProxyInsertion::gather_propagators(
    Map<llvm::Function *, Set<llvm::Value *>> encoded,
    Map<llvm::Function *, Set<llvm::Use *>> to_encode) {

  for (const auto &[func, values] : encoded) {
    for (auto *val : values) {
      for (auto &use : val->uses()) {
        auto *user = dyn_cast<llvm::Instruction>(use.getUser());
        if (not user) {
          continue;
        }

        infoln("GATHER ", *user);

        if (auto *write = into<WriteInst>(user)) {
          if (&use == &write->getValueWrittenAsUse()) {
            this->find_base_object(write->getObject(), *write);
          }

        } else if (auto *insert = into<InsertInst>(user)) {
          if (auto value_keyword = insert->get_keyword<ValueKeyword>()) {
            if (&use == &value_keyword->getValueAsUse()) {
              this->find_base_object(insert->getObject(), *insert);
            }
          }
        }
      }
    }
  }

  for (const auto &[func, uses] : to_encode) {
    for (auto *use : uses) {
      auto *user = dyn_cast<llvm::Instruction>(use->getUser());
      if (not user) {
        continue;
      }

      auto *used = use->get();

      infoln("GATHER ", *user);

      if (auto *arg = dyn_cast<llvm::Argument>(used)) {
        auto &parent = MEMOIR_SANITIZE(arg->getParent(),
                                       "Argument has no parent function.");

        if (auto *fold = FoldInst::get_single_fold(parent)) {
          if (arg == fold->getElementArgument()) {
            this->find_base_object(fold->getObject(), *fold);
          }
        }

      } else if (auto *read = into<ReadInst>(used)) {
        this->find_base_object(read->getObject(), *read);
      }
    }
  }

  // Deduplicate propagators.
  for (auto it = this->propagators.begin(); it != this->propagators.end();) {
    auto &info = *it;

    // Search for an equivalent propagator.
    auto found = std::find_if(this->propagators.begin(),
                              it,
                              [&](const ObjectInfo &other) {
                                return info.allocation == other.allocation
                                       and std::equal(info.offsets.begin(),
                                                      info.offsets.end(),
                                                      other.offsets.begin(),
                                                      other.offsets.end());
                              });

    if (found != it) {
      // If we found an equivalent propagator, delete this one.
      it = this->propagators.erase(it);
    } else {
      // Analyze this object, since it is unique.
      info.analyze();
      // Commit this object.
      ++it;
    }
  }

  return;
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

    println();
    println("FOUND PROPAGATORS ", this->propagators.size());
    for (auto &info : this->propagators) {
      println("  ", info);
    }
    println();
  }

  // Use a heuristic to share proxies between collections.
  Set<const ObjectInfo *> used = {};
  for (auto it = this->objects.begin(); it != this->objects.end(); ++it) {
    auto &info = *it;

    if (used.count(&info)) {
      continue;
    }

    auto *alloc = info.allocation;
    auto *bb = alloc->getParent();
    auto *func = bb->getParent();

    auto &type = MEMOIR_SANITIZE(dyn_cast<AssocType>(&info.get_type()),
                                 "Non-assoc type, unhandled.");

    this->candidates.emplace_back();
    auto &candidate = this->candidates.back();
    candidate.push_back(&info);

    // Find all other allocations in the same function as this one.
    if (not disable_proxy_sharing) {
      for (auto it2 = std::next(it); it2 != this->objects.end(); ++it2) {
        auto &other = *it2;

        if (used.count(&other)) {
          continue;
        }

        // Check that the key types match.
        auto *other_alloc = other.allocation;
        auto &other_type =
            MEMOIR_SANITIZE(dyn_cast<AssocType>(&other.get_type()),
                            "Non-assoc type, unhandled.");

        if (&type.getKeyType() != &other_type.getKeyType()) {
          continue;
        }

        // Check that they share a parent function.
        // NOTE: this is overly conservative
        auto *other_func = other.allocation->getFunction();
        if (func != other_func) {
          continue;
        }

        candidate.push_back(&other);
      }

      // Find all propagators in the same function as this one.
      for (auto &other : this->propagators) {
        if (used.count(&other) > 0) {
          continue;
        }

        auto *other_alloc = other.allocation;
        auto &other_type = other.get_type();

        if (&type.getKeyType() != &other_type) {
          continue;
        }

        // Check that they share a parent basic block.
        auto *other_func = other_alloc->getFunction();
        if (func != other_func) {
          continue;
        }

        candidate.push_back(&other);
      }
    }

    // Compute the benefit of each object in the candidate.
    uint32_t candidate_benefit = 0;
    Set<const ObjectInfo *> has_benefit = {};
    for (const auto *info : candidate) {
      for (const auto *other : candidate) {
        if (info == other) {
          continue;
        }

        auto benefit = info->compute_heuristic(*other);

        if (benefit > 0) {
          has_benefit.insert(info);
          has_benefit.insert(other);
          candidate_benefit += benefit;
        }
      }
    }

    // Remove objects that provide no benefits.
    std::erase_if(candidate, [&](ObjectInfo *info) {
      return has_benefit.count(info) == 0;
    });

    // If there is no benefit, roll back the candidate.
    if (candidate.size() == 0) {
      candidates.pop_back();
    } else {

      // Mark the objects in the candidate as being used.
      used.insert(candidate.begin(), candidate.end());

      println("CANDIDATE:");
      println("  BENEFIT=", candidate_benefit);
      for (const auto *info : candidate) {
        println("  ", *info);
      }
    }
  }
}

} // namespace folio
