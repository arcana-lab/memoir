#include <algorithm>
#include <numeric>

#include "llvm/IR/AttributeMask.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "memoir/ir/Builder.hpp"
#include "memoir/lowering/Implementation.hpp"
#include "memoir/support/WorkList.hpp"
#include "memoir/transforms/utilities/MutateType.hpp"
#include "memoir/transforms/utilities/ReifyTempArgs.hpp"
#include "memoir/utility/Metadata.hpp"

#include "folio/transforms/ProxyInsertion.hpp"

using namespace llvm::memoir;

namespace folio {

static llvm::cl::opt<bool> disable_proxy_propagation(
    "disable-proxy-propagation",
    llvm::cl::desc("Disable proxy propagation"),
    llvm::cl::init(false));

static llvm::cl::opt<bool> disable_use_coalescing(
    "disable-proxy-use-coalescing",
    llvm::cl::desc("Disable coalescing proxy uses"),
    llvm::cl::init(false));

static llvm::Function *parent_function(llvm::Value &V) {
  if (auto *arg = dyn_cast<llvm::Argument>(&V)) {
    return arg->getParent();
  } else if (auto *inst = dyn_cast<llvm::Instruction>(&V)) {
    return inst->getFunction();
  }
  return nullptr;
}

static void update_values(llvm::ValueToValueMapTy &vmap,
                          set<llvm::Value *> &input,
                          set<llvm::Value *> &output) {
  for (auto *val : input) {
    auto *clone = &*vmap[val];

    output.insert(clone);
  }
}

static void update_uses(map<llvm::Function *, set<llvm::Use *>> &uses,
                        llvm::Function &old_func,
                        llvm::Function &new_func,
                        llvm::ValueToValueMapTy &vmap,
                        bool delete_old) {

  if (uses.count(&old_func)) {

    auto &old_uses = uses[&old_func];
    auto &new_uses = uses[&new_func];

    if (delete_old) {
      uses.erase(&old_func);
    }

    for (auto *use : old_uses) {
      auto *user = dyn_cast<llvm::Instruction>(use->getUser());
      auto *clone = dyn_cast<llvm::Instruction>(&*vmap[user]);

      auto &clone_use = clone->getOperandUse(use->getOperandNo());

      new_uses.insert(&clone_use);
    }
  }
}

void ObjectInfo::update(llvm::Function &old_func,
                        llvm::Function &new_func,
                        llvm::ValueToValueMapTy &vmap,
                        bool delete_old) {

  // Unpack the object info.
  auto *alloc = this->allocation;
  auto &inst = alloc->getCallInst();

  // If this allocations parent is the function being cloned, we need
  // to update it.
  if (inst.getFunction() == &old_func) {
    auto *new_inst = &*vmap[&inst];
    auto *new_alloc = into<AllocInst>(new_inst);

    // Update in-place.
    this->allocation = new_alloc;
  }

  // Update the set of redefinitions.
  auto &redefs = this->redefinitions;
  if (redefs.count(&old_func)) {

    update_values(vmap, redefs[&old_func], redefs[&new_func]);

    if (delete_old) {
      redefs.erase(&old_func);
    }
  }

  // Update the uses.
  update_uses(this->to_encode, old_func, new_func, vmap, delete_old);
  update_uses(this->to_addkey, old_func, new_func, vmap, delete_old);

  // Update the set of encoded values.
  update_values(vmap, this->encoded[&old_func], this->encoded[&new_func]);

  if (delete_old) {
    this->encoded[&old_func].clear();
  }

  return;
}

uint32_t ObjectInfo::compute_heuristic(const ObjectInfo &other) const {
  uint32_t benefit = 0;
  for (const auto &[func, encoded] : this->encoded) {
    for (const auto *value : encoded) {

      for (auto &use_to_decode : value->uses()) {

        auto *user = use_to_decode.getUser();

        if (other.to_encode.count(func) > 0) {
          for (const auto *use_to_encode : other.to_encode.at(func)) {
            if (&use_to_decode == use_to_encode) {
              ++benefit;
            }
          }
        }

        if (other.to_addkey.count(func) > 0) {
          for (const auto *use_to_addkey : other.to_addkey.at(func)) {
            if (&use_to_decode == use_to_addkey) {
              ++benefit;
            }
          }
        }

        if (other.encoded.count(func) > 0) {
          auto *cmp = dyn_cast<llvm::CmpInst>(user);
          if (cmp and cmp->isEquality()) {
            if (other.encoded.at(func).count(cmp->getOperand(0))) {
              ++benefit;
            } else if (other.encoded.at(func).count(cmp->getOperand(1))) {
              ++benefit;
            }
          } else if (auto *phi = dyn_cast<llvm::PHINode>(user)) {
            bool all_decode = true;
            for (auto &incoming : phi->incoming_values()) {
              if (incoming == use_to_decode) {
                continue;
              }
              if (other.encoded.at(func).count(incoming.get()) == 0) {
                all_decode = false;
              }
            }

            if (all_decode) {
              ++benefit;
            }
          }
        }
      }
    }
  }

  return benefit;
}

bool ObjectInfo::is_redefinition(llvm::Value &V) const {
  for (const auto &[func, redefs] : this->redefinitions) {
    if (redefs.count(&V) > 0) {
      return true;
    }
  }
  return false;
}

static void gather_redefinitions(
    llvm::Value &V,
    map<llvm::Function *, set<llvm::Value *>> &redefinitions) {

  auto *function = parent_function(V);
  MEMOIR_ASSERT(function, "Unknown parent function for redefinition.");

  if (redefinitions[function].count(&V) > 0) {
    return;
  }

  redefinitions[function].insert(&V);

  for (auto &use : V.uses()) {
    auto *user = use.getUser();

    // Recurse on redefinitions.
    if (auto *phi = dyn_cast<llvm::PHINode>(user)) {
      gather_redefinitions(*user, redefinitions);

    } else if (auto *memoir_inst = into<MemOIRInst>(user)) {
      if (auto *update = dyn_cast<UpdateInst>(memoir_inst)) {
        if (&use == &update->getObjectAsUse()) {
          gather_redefinitions(*user, redefinitions);
        }

      } else if (isa<RetPHIInst>(memoir_inst) or isa<UsePHIInst>(memoir_inst)) {

        // Recurse on redefinitions.
        gather_redefinitions(*user, redefinitions);
      }

      // Gather variable if folded on, or recurse on closed argument.
      else if (auto *fold = into<FoldInst>(user)) {

        if (use == fold->getInitialAsUse()) {
          // Gather uses of the accumulator argument.
          gather_redefinitions(fold->getAccumulatorArgument(), redefinitions);

          // Gather uses of the resultant.
          gather_redefinitions(fold->getResult(), redefinitions);

        } else if (use == fold->getObjectAsUse()) {
          // Do nothing.

        } else if (auto *closed_arg = fold->getClosedArgument(use)) {
          // Gather uses of the closed argument.
          gather_redefinitions(*closed_arg, redefinitions);
        }
      }

    } else if (auto *call = dyn_cast<llvm::CallBase>(user)) {
      auto &callee =
          MEMOIR_SANITIZE(call->getCalledFunction(),
                          "Found use of MEMOIR collection by indirect callee!");

      auto &arg =
          MEMOIR_SANITIZE(callee.getArg(use.getOperandNo()),
                          "No arguments in the callee matching this use!");

      gather_redefinitions(arg, redefinitions);
    }
  }

  return;
}

static bool is_last_index(llvm::Use *use,
                          AccessInst::index_op_iterator index_end) {
  return std::next(AccessInst::index_op_iterator(use)) == index_end;
}

static llvm::Use *get_index_use(AccessInst &access,
                                llvm::ArrayRef<unsigned> offsets) {

  auto offset_it = offsets.begin();
  auto offset_ie = offsets.end();

  auto index_it = access.index_operands_begin();
  auto index_ie = access.index_operands_end();

  auto *type = &access.getObjectType();

  for (auto offset : offsets) {

    // If we have reached the end of the index operands, there is no index
    // use.
    if (index_it == index_ie) {
      return nullptr;
    }

    if (auto *struct_type = dyn_cast<TupleType>(type)) {

      auto &index_use = *index_it;
      auto &index_const =
          MEMOIR_SANITIZE(dyn_cast<llvm::ConstantInt>(index_use.get()),
                          "Field index is not statically known!");

      // If the offset doesn't match the field index, there is no index use.
      if (offset != index_const.getZExtValue()) {
        return nullptr;
      }

      // Get the inner type.
      type = &struct_type->getFieldType(offset);

    } else if (auto *collection_type = dyn_cast<CollectionType>(type)) {
      // Get the inner type.
      type = &collection_type->getElementType();
    }

    ++index_it;
  }

  // If we are at the end of the index operands, return NULL.
  if (index_it == index_ie) {
    return nullptr;
  }

  // Otherwise, fetch the index use and return it.
  auto *index_use = &*index_it;

  return index_it;
}

static void gather_uses_to_proxy(
    llvm::Value &V,
    llvm::ArrayRef<unsigned> offsets,
    map<llvm::Function *, set<llvm::Value *>> &encoded,
    map<llvm::Function *, set<llvm::Use *>> &to_encode,
    map<llvm::Function *, set<llvm::Use *>> &to_addkey) {

  infoln("REDEF ", V);

  auto *function = parent_function(V);
  MEMOIR_ASSERT(function, "Gathering uses of value with no parent function!");

  // From a given collection, V, gather all uses that need to be either
  // encoded or decoded.
  for (auto &use : V.uses()) {
    auto *user = use.getUser();

    infoln("  USER ", *user);

    if (auto *fold = into<FoldInst>(user)) {

      if (use == fold->getObjectAsUse()) {

        // If we find an index use, encode it.
        if (auto *index_use = get_index_use(*fold, offsets)) {
          infoln("    ENCODING INDEX");
          to_encode[function].insert(index_use);

        } else {

          // If the offset is exactly equal to the keys being folded over,
          // decode the index argument of the body.
          auto distance = fold->match_offsets(offsets);

          if (not distance) {
            // Do nothing.
          }

          // If the offsets are fully exhausted, add uses of the index
          // argument to the set of uses to decode.
          else if (distance.value() == offsets.size()) {
            auto &index_arg = fold->getIndexArgument();
            infoln("    DECODING KEY");
            encoded[&fold->getBody()].insert(&index_arg);
          }

          // If the offsets are not fully exhausted, recurse on the value
          // argument.
          else if (distance.value() < offsets.size()) {
            if (auto *elem_arg = fold->getElementArgument()) {
              infoln("    RECURSING");
              gather_uses_to_proxy(*elem_arg,
                                   offsets.drop_front(distance.value() + 1),
                                   encoded,
                                   to_encode,
                                   to_addkey);
            }
          }
        }
      }

    } else if (auto *access = into<AccessInst>(user)) {

      if (use == access->getObjectAsUse()) {
        // Find the index use for the given offset and mark it for decoding.
        if (auto *index_use = get_index_use(*access, offsets)) {
          if (isa<InsertInst>(access)) {
            if (is_last_index(index_use, access->index_operands_end())) {
              infoln("    ADDING KEY ", *index_use->get());
              to_addkey[function].insert(index_use);
              continue;
            }
          }

          infoln("    ENCODING KEY ", *index_use->get());
          to_encode[function].insert(index_use);
        }
      }
    }
  }

  return;
}

static void gather_uses_to_propagate(
    llvm::Value &V,
    llvm::ArrayRef<unsigned> offsets,
    map<llvm::Function *, set<llvm::Value *>> &encoded,
    map<llvm::Function *, set<llvm::Use *>> &to_encode,
    map<llvm::Function *, set<llvm::Use *>> &to_addkey) {

  infoln("REDEF ", V, " IN ", parent_function(V)->getName());

  auto *function = parent_function(V);
  MEMOIR_ASSERT(function, "Gathering uses of value with no parent function!");

  // From a given collection, V, gather all uses that need to be either
  // encoded or decoded.
  for (auto &use : V.uses()) {
    auto *user = dyn_cast<llvm::Instruction>(use.getUser());
    if (not user) {
      continue;
    }

    infoln("  USER ", *user);

    if (auto *access = into<AccessInst>(user)) {

      // Ensure that the use is the object being accessed.
      if (use != access->getObjectAsUse()) {
        continue;
      }

      // Try to match the access indices against the offsets.
      auto maybe_distance = access->match_offsets(offsets);

      // If the indices don't match, skip.
      if (not maybe_distance) {
        continue;
      }

      auto distance = maybe_distance.value();

      if (auto *fold = dyn_cast<FoldInst>(access)) {
        if (use == fold->getObjectAsUse()) {
          // If the offsets are fully exhausted, add uses of the index
          // argument to the set of uses to decode.
          // TODO: may need to do size+1
          if ((distance + 1) == offsets.size()) {
            if (auto *elem_arg = fold->getElementArgument()) {
              infoln("    DECODING ELEM");
              encoded[&fold->getBody()].insert(elem_arg);
            }
          }

          // If the offsets are not fully exhausted, recurse on the value
          // argument.
          else if (distance < offsets.size()) {
            if (auto *elem_arg = fold->getElementArgument()) {
              infoln("    RECURSING");
              gather_uses_to_propagate(*elem_arg,
                                       offsets.drop_front(distance + 1),
                                       encoded,
                                       to_encode,
                                       to_addkey);
            }
          }
        }

      } else if (auto *read = dyn_cast<ReadInst>(access)) {
        if (distance == offsets.size()) {
          infoln("    DECODING ELEM ");
          encoded[function].insert(&read->asValue());
        }

      } else if (auto *write = dyn_cast<WriteInst>(access)) {
        if (distance == offsets.size()) {
          infoln("    ADDKEY ");
          to_addkey[function].insert(&write->getValueWrittenAsUse());
        }

      } else if (auto *insert = dyn_cast<InsertInst>(access)) {
        if (auto value_kw = insert->get_keyword<ValueKeyword>()) {
          if (distance == offsets.size()) {
            infoln("    ADDKEY ");
            to_addkey[function].insert(&value_kw->getValueAsUse());
          }
        }
      }
    }
  }

  return;
}

bool ObjectInfo::is_propagator() const {
  return not isa<CollectionType>(&this->get_type());
}

void ObjectInfo::analyze() {
  infoln();
  infoln("ANALYZING ", *this);

  gather_redefinitions(this->allocation->getCallInst(), this->redefinitions);

  bool is_propagator = this->is_propagator();

  for (const auto &[func, redefs] : this->redefinitions) {
    for (auto *redef : redefs) {
      if (is_propagator) {
        gather_uses_to_propagate(*redef,
                                 this->offsets,
                                 this->encoded,
                                 this->to_encode,
                                 this->to_addkey);
      } else {
        gather_uses_to_proxy(*redef,
                             this->offsets,
                             this->encoded,
                             this->to_encode,
                             this->to_addkey);
      }
    }
  }

#if 0
  for (const auto &[func, redefs] : this->redefinitions) {
    infoln("IN ", func->getName());
    for (auto *redef : redefs) {
      infoln("  REDEF ", *redef);
    }
  }
#endif

  infoln();
}

Type &ObjectInfo::get_type() const {
  auto *type = &this->allocation->getType();
  for (auto offset : this->offsets) {
    if (auto *struct_type = dyn_cast<TupleType>(type)) {
      type = &struct_type->getFieldType(offset);
    } else if (auto *collection_type = dyn_cast<CollectionType>(type)) {
      type = &collection_type->getElementType();
    }
  }

  return *type;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const ObjectInfo &info) {
  os << "(" << *info.allocation << ")";
  for (auto offset : info.offsets) {
    if (offset == unsigned(-1)) {
      os << "[*]";
    } else {
      os << "." << std::to_string(offset);
    }
  }

  return os;
}

void ProxyInsertion::gather_assoc_objects(vector<ObjectInfo> &allocations,
                                          AllocInst &alloc,
                                          Type &type,
                                          vector<unsigned> offsets) {

  if (auto *struct_type = dyn_cast<TupleType>(&type)) {
    for (unsigned field = 0; field < struct_type->getNumFields(); ++field) {

      auto new_offsets = offsets;
      new_offsets.push_back(field);

      this->gather_assoc_objects(allocations,
                                 alloc,
                                 struct_type->getFieldType(field),
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
                                    vector<unsigned> &offsets,
                                    set<llvm::Value *> &visited) {
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

  vector<unsigned> offsets = {};
  for (auto *index : access.indices()) {
    if (auto *struct_type = dyn_cast<TupleType>(type)) {
      auto index_const = dyn_cast<llvm::ConstantInt>(index);
      auto field = index_const->getZExtValue();
      type = &struct_type->getFieldType(field);

      offsets.push_back(field);

    } else if (auto *collection_type = dyn_cast<CollectionType>(type)) {
      type = &collection_type->getElementType();

      offsets.push_back(-1);
    }
  }
  if (isa<FoldInst>(&access)) {
    offsets.push_back(-1);
  }

  set<llvm::Value *> visited = {};

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
    map<llvm::Function *, set<llvm::Value *>> encoded,
    map<llvm::Function *, set<llvm::Use *>> to_encode) {

  for (const auto &[func, values] : encoded) {
    for (auto *val : values) {
      for (auto &use : val->uses()) {
        auto *user = dyn_cast<llvm::Instruction>(use.getUser());
        if (not user) {
          continue;
        }

        infoln("GATHER ", *user);

        if (auto *write = into<WriteInst>(user)) {
          if (use == write->getValueWrittenAsUse()) {
            this->find_base_object(write->getObject(), *write);
          }

        } else if (auto *insert = into<InsertInst>(user)) {
          if (auto value_keyword = insert->get_keyword<ValueKeyword>()) {
            if (use == value_keyword->getValueAsUse()) {
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

    // If we found an equivalent propagator, delete this one.
    if (found != it) {
      it = this->propagators.erase(it);
    } else {

      // Analyze this object, since it is unique.
      info.analyze();

      ++it;
    }
  }

  return;
}

static uint32_t forward_analysis(
    map<llvm::Function *, set<llvm::Value *>> &encoded) {

  uint32_t count = 0;

  WorkList<llvm::Value *> worklist;

  for (auto &[func, values] : encoded) {
    worklist.push(values.begin(), values.end());
  }

  while (not worklist.empty()) {
    auto *val = worklist.pop();

    auto *func = parent_function(*val);
    auto &local_encoded = encoded[func];

    for (auto &use : val->uses()) {
      auto *user = use.getUser();

      if (local_encoded.count(user) > 0) {
        continue;
      }

      if (auto *phi = dyn_cast<llvm::PHINode>(user)) {
        bool all_encoded = true;
        for (auto &incoming : phi->incoming_values()) {
          if (local_encoded.count(incoming.get()) == 0) {
            all_encoded = false;
            break;
          }
        }

        if (all_encoded) {
          local_encoded.insert(phi);
          worklist.push(phi);
          ++count;
        }

      } else if (auto *select = dyn_cast<llvm::SelectInst>(user)) {
        auto *lhs = select->getTrueValue();
        auto *rhs = select->getFalseValue();

        if (local_encoded.count(lhs) and local_encoded.count(rhs)) {
          local_encoded.insert(select);
          worklist.push(select);
          ++count;
        }

      } else if (auto *fold = into<FoldInst>(user)) {
        auto &body = fold->getBody();
        auto &body_encoded = encoded[&body];

        if (use == fold->getInitialAsUse()) {
          auto &accum_arg = fold->getAccumulatorArgument();

          if (body_encoded.count(&accum_arg) > 0) {
            continue;
          }

          // Check if the return value of the fold is encoded.
          bool return_encoded = true;
          for (auto &BB : body) {
            if (auto *ret = dyn_cast<llvm::ReturnInst>(BB.getTerminator())) {
              auto *ret_val = ret->getReturnValue();
              if (local_encoded.count(ret_val) == 0) {
                return_encoded = false;
              }
            }
          }

          if (return_encoded) {
            body_encoded.insert(&accum_arg);
            worklist.push(&accum_arg);
            ++count;
          }

        } else if (auto *closed_arg = fold->getClosedArgument(use)) {
          if (body_encoded.count(closed_arg) > 0) {
            continue;
          }

          body_encoded.insert(closed_arg);
          worklist.push(closed_arg);
          ++count;
        }
      }
    }
  }

  return count;
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
    map<llvm::Function *, set<llvm::Value *>> encoded = {};
    map<llvm::Function *, set<llvm::Use *>> to_encode = {};
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

  // Use a heuristic to group together objects.
  set<const ObjectInfo *> used = {};
  for (auto it = this->objects.begin(); it != this->objects.end(); ++it) {
    auto &info = *it;

    if (used.count(&info) > 0) {
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
    for (auto it2 = std::next(it); it2 != this->objects.end(); ++it2) {
      auto &other = *it2;

      if (used.count(&other) > 0) {
        continue;
      }

      // Check that the key types match.
      auto *other_alloc = other.allocation;
      auto &other_type = MEMOIR_SANITIZE(dyn_cast<AssocType>(&other.get_type()),
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

    // Compute the benefit of each object in the candidate.
    uint32_t candidate_benefit = 0;
    set<const ObjectInfo *> has_benefit = {};
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

static llvm::FunctionCallee create_addkey_function(llvm::Module &M,
                                                   Type &key_type,
                                                   bool build_encoder,
                                                   Type *encoder_type,
                                                   bool build_decoder,
                                                   Type *decoder_type) {

  auto &context = M.getContext();
  auto &data_layout = M.getDataLayout();

  auto &size_type = Type::get_size_type(data_layout);

  auto *llvm_size_type = size_type.get_llvm_type(context);
  auto *llvm_ptr_type = llvm::PointerType::get(context, 0);
  auto *llvm_key_type = key_type.get_llvm_type(context);

  // Create the addkey functions for this proxy.
  vector<llvm::Type *> addkey_params = { llvm_key_type };
  if (build_encoder) {
    addkey_params.push_back(llvm_ptr_type);
  }
  if (build_decoder) {
    addkey_params.push_back(llvm_ptr_type);
  }
  auto *addkey_type =
      llvm::FunctionType::get(llvm_size_type, addkey_params, false);
  auto &addkey_function = MEMOIR_SANITIZE(
      llvm::Function::Create(addkey_type,
                             llvm::GlobalValue::LinkageTypes::InternalLinkage,
                             "proxy_addkey_",
                             M),
      "Failed to create LLVM function");

  auto arg_idx = 0;
  auto *key = addkey_function.getArg(arg_idx++);

  llvm::Argument *encoder = nullptr;
  if (build_encoder) {
    encoder = addkey_function.getArg(arg_idx++);
  }

  llvm::Argument *decoder = nullptr;
  if (build_decoder) {
    decoder = addkey_function.getArg(arg_idx++);
  }

  auto *ret_bb = llvm::BasicBlock::Create(context, "", &addkey_function);

  MemOIRBuilder builder(ret_bb);

  if (build_encoder) {
    builder.CreateAssertTypeInst(encoder, *encoder_type);
  }

  if (build_decoder) {
    builder.CreateAssertTypeInst(decoder, *decoder_type);
  }

  auto *has_key = &builder.CreateHasInst(encoder, key)->getCallInst();
  auto *phi = builder.CreatePHI(llvm_size_type, 2);
  llvm::PHINode *encoder_phi = nullptr;
  if (build_encoder) {
    encoder_phi = builder.CreatePHI(llvm_ptr_type, 2);
  }

  llvm::PHINode *decoder_phi = nullptr;
  if (build_decoder) {
    decoder_phi = builder.CreatePHI(llvm_ptr_type, 2);
  }

  auto *ret = builder.CreateRet(phi);

  llvm::Instruction *then_terminator, *else_terminator;
  llvm::SplitBlockAndInsertIfThenElse(has_key,
                                      phi,
                                      &then_terminator,
                                      &else_terminator);

  // if (has(encoder, key)) {
  //   z2 = read(encoder, key)
  // }
  auto *then_bb = then_terminator->getParent();
  builder.SetInsertPoint(then_terminator);
  auto *read_index =
      &builder.CreateReadInst(size_type, encoder, { key })->getCallInst();

  // else {
  //   z1 = size(encoder)
  //   insert(size(encoder), encoder, key)
  //   insert(key, decoder, end)
  // }
  auto *else_bb = else_terminator->getParent();
  builder.SetInsertPoint(else_terminator);
  llvm::Value *new_index = nullptr;
  llvm::Value *new_encoder = nullptr;
  if (encoder) {
    new_index = &builder.CreateSizeInst(encoder)->getCallInst();
    auto *insert = builder.CreateInsertInst(encoder, { key });
    new_encoder = &builder
                       .CreateWriteInst(size_type,
                                        new_index,
                                        &insert->getCallInst(),
                                        { key })
                       ->getCallInst();
  }

  llvm::Value *new_decoder = nullptr;
  if (decoder) {
    if (not new_index) {
      new_index = &builder.CreateSizeInst(decoder)->getCallInst();
    }
    auto *end = &builder.CreateEndInst()->getCallInst();
    auto *insert = builder.CreateInsertInst(decoder, { end });
    new_decoder = &builder
                       .CreateWriteInst(key_type,
                                        key,
                                        &insert->getCallInst(),
                                        { new_index })
                       ->getCallInst();
  }

  // Update the PHIs
  phi->addIncoming(read_index, then_bb);
  phi->addIncoming(new_index, else_bb);

  if (encoder_phi) {
    encoder_phi->addIncoming(encoder, then_bb);
    encoder_phi->addIncoming(new_encoder, else_bb);
    auto encoder_live_out = Metadata::get_or_add<LiveOutMetadata>(*encoder_phi);
    encoder_live_out.setArgNo(encoder->getArgNo());
  }

  if (decoder_phi) {
    decoder_phi->addIncoming(decoder, then_bb);
    decoder_phi->addIncoming(new_decoder, else_bb);
    auto decoder_live_out = Metadata::get_or_add<LiveOutMetadata>(*decoder_phi);
    decoder_live_out.setArgNo(decoder->getArgNo());
  }

  return llvm::FunctionCallee(&addkey_function);
}

static void collect_callers(llvm::Function &to,
                            set<llvm::Function *> &functions) {

  if (functions.count(&to) > 0) {
    return;
  }

  functions.insert(&to);

  // Collect all direct callers of this function.
  for (auto &use : to.uses()) {
    auto *call = dyn_cast<llvm::CallBase>(use.getUser());
    if (not call) {
      continue;
    }
    auto *memoir = into<MemOIRInst>(call);
    auto *fold = dyn_cast_or_null<FoldInst>(memoir);
    if (memoir and not fold) {
      continue;
    }

    // Ensure that this is the function being called.
    auto *callee = fold ? &fold->getBody() : call->getCalledFunction();
    if (callee != &to) {
      continue;
    }

    // Insert the parent function.
    collect_callers(*call->getFunction(), functions);
  }
}

static void collect_callees(llvm::Function &from,
                            set<llvm::Function *> &functions) {
  if (functions.count(&from) > 0) {
    return;
  }

  functions.insert(&from);

  // Collect all direct callees from this function.
  for (auto &BB : from) {
    for (auto &I : BB) {
      auto *call = dyn_cast<llvm::CallBase>(&I);
      if (not call) {
        continue;
      }
      auto *memoir = into<MemOIRInst>(call);
      auto *fold = dyn_cast_or_null<FoldInst>(memoir);
      if (memoir and not fold) {
        continue;
      }

      auto *callee = fold ? &fold->getBody() : call->getCalledFunction();

      // Insert the called function.
      if (callee) {
        collect_callees(*callee, functions);
      }
    }
  }
}

static void add_tempargs(
    map<llvm::Function *, llvm::Instruction *> &local_patches,
    llvm::ArrayRef<set<llvm::Use *>> uses_to_patch,
    llvm::Instruction &patch_with,
    Type &patch_type,
    const llvm::Twine &name) {

  MemOIRBuilder builder(&patch_with);
  auto &context = builder.getContext();
  auto &module = builder.getModule();

  // Unpack the patch.
  auto *patch_func = patch_with.getFunction();
  auto *llvm_patch_type = patch_type.get_llvm_type(context);

  // Track the local patch for each function.
  local_patches[patch_func] = &patch_with;

  // Find the set of functions that need the patch.
  set<llvm::Function *> functions = { patch_func };
  for (auto &uses : uses_to_patch) {
    for (auto *use : uses) {
      if (auto *inst = dyn_cast<llvm::Instruction>(use->getUser())) {
        functions.insert(inst->getFunction());
      }
    }
  }

  // Close the set of functions.
  set<llvm::Function *> forward = {};
  set<llvm::Function *> backward = {};
  for (auto *func : functions) {
    collect_callers(*func, forward);
    collect_callees(*func, backward);
  }

  for (auto *func : forward) {
    if (backward.count(func) > 0) {
      functions.insert(func);
    }
  }

  // Determine the set of functions that we need to pass the proxy to.
  map<llvm::CallBase *, llvm::GlobalVariable *> calls_to_patch = {};
  map<FoldInst *, llvm::GlobalVariable *> folds_to_patch = {};
  for (auto *func : functions) {

    if (func == patch_func) {
      continue;
    }

    // Create the global variable.
    auto *global = new llvm::GlobalVariable(
        module,
        llvm_patch_type,
        /* isConstant? */ false,
        llvm::GlobalValue::LinkageTypes::InternalLinkage,
        llvm::Constant::getNullValue(llvm_patch_type),
        "temparg");

    // Create the load in the entry of the fold body.
    builder.SetInsertPoint(func->getEntryBlock().getFirstNonPHI());

    auto *load = builder.CreateLoad(llvm_patch_type, global);
    Metadata::get_or_add<TempArgumentMetadata>(*load);

    load->setName(name);

    // Annotate the loaded value with type information.
    builder.CreateAssertTypeInst(load, patch_type);

    local_patches[func] = load;

    for (auto &use : func->uses()) {
      auto *call = dyn_cast<llvm::CallBase>(use.getUser());
      if (not call) {
        continue;
      }

      if (auto *fold = into<FoldInst>(call)) {
        if (&fold->getBody() == func) {
          folds_to_patch[fold] = global;
        }
      } else if (not into<MemOIRInst>(call)) {
        if (call->getCalledFunction() == func) {
          calls_to_patch[call] = global;
        }
      }
    }
  }

  // Patch each of the folds by storing to the global before the operation.
  for (const auto &[fold, global] : folds_to_patch) {
    // Unpack.
    auto &inst = fold->getCallInst();
    auto *func = inst.getFunction();

    // Create the store ahead of the fold.
    builder.SetInsertPoint(&inst);

    auto *local = local_patches[func];

    if (not local) {
      warnln("No local patch for caller ", func->getName());
      continue;
    }

    auto *store = builder.CreateStore(local, global);
    Metadata::get_or_add<TempArgumentMetadata>(*store);
  }

  // Patch each of the calls by storing to the global before the operation.
  for (const auto &[call, global] : calls_to_patch) {
    // Unpack.
    auto *func = call->getFunction();

    // Create the store ahead of the call.
    builder.SetInsertPoint(call);

    auto *local = local_patches[func];

    if (not local) {
      warnln("No local patch for caller ", func->getName());
      continue;
    }

    auto *store = builder.CreateStore(local, global);
    Metadata::get_or_add<TempArgumentMetadata>(*store);
  }

  return;
}

/**
 * Following a function clone--where the old function will be deleted--update
 * any candidate information to point to the cloned function.
 */
static void update_candidates(std::forward_iterator auto candidates_begin,
                              std::forward_iterator auto candidates_end,
                              llvm::Function &old_func,
                              llvm::Function &new_func,
                              llvm::ValueToValueMapTy &vmap) {
  for (auto it = candidates_begin; it != candidates_end; ++it) {
    auto &candidate = *it;
    for (auto *info : candidate) {

      info->update(old_func, new_func, vmap, /* delete old? */ true);
    }
  }
}

static bool used_value_will_be_decoded(llvm::Use &use,
                                       const set<llvm::Use *> &to_decode,
                                       set<llvm::Use *> &visited) {

  debugln("DECODED? ", *use.get());

  if (to_decode.count(&use) > 0) {
    debugln("  YES");
    return true;
  }

  if (visited.count(&use) > 0) {
    debugln("  YES");
    return true;
  } else {
    visited.insert(&use);
  }

  auto *value = use.get();
  if (auto *phi = dyn_cast<llvm::PHINode>(value)) {
    debugln("  RECURSE");
    bool all_decoded = true;
    for (auto &incoming : phi->incoming_values()) {
      all_decoded &= used_value_will_be_decoded(incoming, to_decode, visited);
    }
    return all_decoded;
  } else if (auto *select = dyn_cast<llvm::SelectInst>(value)) {
    debugln("  RECURSE");
    return used_value_will_be_decoded(select->getOperandUse(1),
                                      to_decode,
                                      visited)
           and used_value_will_be_decoded(select->getOperandUse(2),
                                          to_decode,
                                          visited);
  } else if (auto *arg = dyn_cast<llvm::Argument>(value)) {
    auto &func =
        MEMOIR_SANITIZE(arg->getParent(), "Argument has no parent function!");
    if (auto *fold = FoldInst::get_single_fold(func)) {
      if (auto *operand_use = fold->getOperandForArgument(*arg)) {
        debugln("  RECURSE");
        return used_value_will_be_decoded(*operand_use, to_decode, visited);
      }
    }
  }

  debugln("  NO");
  return false;
}

static bool used_value_will_be_decoded(llvm::Use &use,
                                       const set<llvm::Use *> &to_decode) {
  set<llvm::Use *> visited = {};
  return used_value_will_be_decoded(use, to_decode, visited);
}

using GroupedUses =
    map<llvm::Function *, map<llvm::Value *, vector<llvm::Use *>>>;
static GroupedUses groupby_function_and_used(const set<llvm::Use *> &uses) {

  GroupedUses local;

  for (auto *use : uses) {
    auto *user = dyn_cast<llvm::Instruction>(use->getUser());
    if (not user) {
      warnln("Non-instruction user found during ProxyInsertion, unexpected.");
      continue;
    }

    auto *func = user->getFunction();

    auto *used = use->get();

    local[func][used].push_back(use);
  }

  return local;
}

struct CoalescedUses : public vector<llvm::Use *> {
protected:
  using Base = vector<llvm::Use *>;

  llvm::Value *_value;

public:
  CoalescedUses(llvm::Use *use) : Base{ use }, _value(use->get()) {}
  CoalescedUses(llvm::ArrayRef<llvm::Use *> uses)
    : Base(uses.begin(), uses.end()),
      _value(uses.front()->get()) {}

  llvm::Value &value() const {
    return *this->_value;
  }

  void value(llvm::Value &value) {
    this->_value = &value;
  }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const CoalescedUses &uses) {

    auto &value = uses.value();

    os << value << " IN " << parent_function(value)->getName() << "\n";

    for (auto *use : uses) {
      os << "  OP" << use->getOperandNo() << " IN " << *use->getUser() << "\n";
    }

    return os;
  }
};

static void sort_in_level_order(vector<llvm::Use *> &uses,
                                llvm::DominatorTree &DT) {

  // First, sort the uses in level order of the dominator tree.
  std::sort(uses.begin(), uses.end(), [&DT](llvm::Use *lhs, llvm::Use *rhs) {
    // Get the user instructions.
    auto *lhs_inst = cast<llvm::Instruction>(lhs->getUser());
    auto *rhs_inst = cast<llvm::Instruction>(rhs->getUser());

    // Get the parent basic blocks.
    auto *lhs_block = lhs_inst->getParent();
    auto *rhs_block = rhs_inst->getParent();

    // Partial domtree level order between basic block.
    auto *lhs_node = DT[lhs_block];
    auto *rhs_node = DT[rhs_block];

    MEMOIR_ASSERT(lhs_node, "LHS NODE = NULL");
    MEMOIR_ASSERT(rhs_node, "RHS NODE = NULL");

    auto lhs_level = lhs_node->getLevel();
    auto rhs_level = rhs_node->getLevel();

    if (lhs_level < rhs_level) {
      return true;
    } else if (rhs_level < lhs_level) {
      return false;
    }

    if (lhs_block == rhs_block) {
      return lhs_inst->comesBefore(rhs_inst);
    }

    return lhs_block < rhs_block;
  });

  return;
}

static void coalesce_by_dominance(
    vector<CoalescedUses> &coalesced,
    GroupedUses &grouped,
    ProxyInsertion::GetDominatorTree get_dominator_tree) {

  for (auto &[func, locals] : grouped) {

    // Fetch the dominators for this function.
    auto &DT = get_dominator_tree(*func);

    // For each of the local values being decoded:
    for (auto &[val, uses] : locals) {

      // Special handling for values with single uses.
      if (uses.size() == 1) {
        coalesced.emplace_back(uses);
        continue;
      }

      // Sort the uses in level order of the dominator tree.
      sort_in_level_order(uses, DT);

      // Group together uses that are dominated by one another.
      set<llvm::Use *> visited = {};
      for (auto it = uses.begin(); it != uses.end(); ++it) {
        auto *use = *it;

        if (visited.count(use) > 0) {
          continue;
        } else {
          visited.insert(use);
        }

        // Unpack the use.
        auto *user = cast<llvm::Instruction>(use->getUser());

        // Add a new coalesced use.
        coalesced.emplace_back(use);
        auto &current = coalesced.back();

        // If coalescing is disabled, then don't!
        if (disable_use_coalescing) {
          continue;
        }

        // Don't coalesce has operations.
        if (into<HasInst>(user)) {
          continue;
        }

        // Try to coalesce the remaining uses.
        for (auto it2 = std::next(it); it2 != uses.end(); ++it2) {
          auto *other_use = *it2;
          auto *other_user = cast<llvm::Instruction>(other_use->getUser());

          if (DT.dominates(user, *other_use)) {
            current.push_back(other_use);
            visited.insert(other_use);

          } else if (DT.dominates(other_user, *use)) {
            // This check is unnecessary, it's here as a sanity check.
            MEMOIR_UNREACHABLE("Level order is incorrect!\n",
                               "      ",
                               *other_user,
                               " doms ",
                               *user);
          }
        }
      }
    }
  }

  for (auto &uses : coalesced) {
    println("COALESCED ", uses);
  }

  return;
}

static void coalesce(vector<CoalescedUses> &decoded,
                     vector<CoalescedUses> &encoded,
                     vector<CoalescedUses> &added,
                     const set<llvm::Use *> &to_decode,
                     const set<llvm::Use *> &to_encode,
                     const set<llvm::Use *> &to_addkey,
                     ProxyInsertion::GetDominatorTree get_dominator_tree) {

  // Group uses by their parent function and the value being used..
  auto grouped_to_decode = groupby_function_and_used(to_decode);
  auto grouped_to_encode = groupby_function_and_used(to_encode);
  auto grouped_to_addkey = groupby_function_and_used(to_addkey);

  // For each function with a use that needs decoding:
  println("COALESCE TO DECODE");
  coalesce_by_dominance(decoded, grouped_to_decode, get_dominator_tree);
  println();

  println("COALESCE TO ENCODE");
  coalesce_by_dominance(encoded, grouped_to_encode, get_dominator_tree);
  println();

  println("COALESCE TO ADDKEY");
  coalesce_by_dominance(added, grouped_to_addkey, get_dominator_tree);
  println();

  // Further coalesce addkey and encoded uses.
  // TODO

  return;
}

static void update_use(llvm::Use &use, llvm::Value &value) {

  use.set(&value);

  if (auto *call = dyn_cast<llvm::CallBase>(use.getUser())) {
    auto operand_no = use.getOperandNo();
    if (operand_no < call->arg_size()) {
      call->removeParamAttr(operand_no, llvm::Attribute::AttrKind::NonNull);
    }
  }

  return;
}

static llvm::Value &encode_use(
    MemOIRBuilder &builder,
    llvm::Use &use,
    std::function<llvm::Value &(llvm::Value &)> get_encoder,
    std::function<llvm::Value &(MemOIRBuilder &, llvm::Value &)> encode_value) {

  auto *user = cast<llvm::Instruction>(use.getUser());
  auto *used = use.get();

  auto &module = MEMOIR_SANITIZE(user->getModule(), "User has no module.");
  auto &data_layout = module.getDataLayout();
  auto &context = user->getContext();

  auto &size_type = Type::get_size_type(data_layout);
  auto &llvm_size_type = *size_type.get_llvm_type(context);

  auto &encoder = get_encoder(*user);

  // Handle has operations separately.
  if (auto *has = into<HasInst>(user)) {
    if (is_last_index(&use, has->index_operands_end())) {
      // Construct an if-else block.
      // if (has(encoder, key))
      //   i = read(encoder, key)
      //   h = has(collection, i)
      // h' = PHI(h, false)

      auto *cond = &builder.CreateHasInst(&encoder, used)->getCallInst();
      auto *phi = builder.CreatePHI(cond->getType(), 2);

      // i' = PHI(i, undef)
      auto *index_phi = builder.CreatePHI(&llvm_size_type, 2);

      auto *then_terminator =
          llvm::SplitBlockAndInsertIfThen(cond,
                                          phi,
                                          /* unreachable? */ false);

      user->moveBefore(then_terminator);

      builder.SetInsertPoint(user);

      auto &encoded = encode_value(builder, *used);

      update_use(use, encoded);

      user->replaceAllUsesWith(phi);

      auto *then_bb = then_terminator->getParent();
      auto *else_bb = cond->getParent();

      phi->addIncoming(user, then_bb);
      phi->addIncoming(llvm::ConstantInt::getFalse(context), else_bb);

      index_phi->addIncoming(&encoded, then_bb);
      index_phi->addIncoming(llvm::UndefValue::get(&llvm_size_type), else_bb);

      return *index_phi;
    }
  }

  // In the common case, read the encoded value and update the use with
  // it.
  auto &encoded = encode_value(builder, *used);

  update_use(use, encoded);

  return encoded;
}

static void inject(
    llvm::LLVMContext &context,
    vector<CoalescedUses> &decoded,
    vector<CoalescedUses> &encoded,
    vector<CoalescedUses> &added,
    std::function<llvm::Value &(llvm::Value &)> get_encoder,
    std::function<llvm::Value &(MemOIRBuilder &, llvm::Value &)> decode_value,
    std::function<llvm::Value &(MemOIRBuilder &, llvm::Value &)> encode_value,
    std::function<llvm::Value &(MemOIRBuilder &, llvm::Value &)> add_value) {

  for (auto &uses : encoded) {

    // Unpack the use.
    auto *use = uses.front();
    auto *used = use->get();
    auto *user = dyn_cast<llvm::Instruction>(use->getUser());

    // Compute the insertion point.
    llvm::Instruction *program_point = nullptr;
    if (user) {
      if (auto *phi = dyn_cast<llvm::PHINode>(user)) {
        MEMOIR_UNREACHABLE("En/decoding uses by PHI is unhandled.");
      } else {
        program_point = user;
      }

    } else {
      MEMOIR_UNREACHABLE("Failed to find a point to encode the value!");
    }

    // Create the builder.
    MemOIRBuilder builder(program_point);

    auto &encoded = encode_use(builder, *use, get_encoder, encode_value);

    uses.value(encoded);

    // Update the coalesced uses.
    for (auto *other_use : uses) {
      if (other_use == use) {
        continue;
      }

      update_use(*other_use, encoded);
    }
  }

  for (auto &uses : added) {

    // Unpack the use.
    auto *use = uses.front();
    auto *used = use->get();
    auto *user = dyn_cast<llvm::Instruction>(use->getUser());

    // Compute the insertion point.
    llvm::Instruction *program_point = nullptr;
    if (user) {
      if (auto *phi = dyn_cast<llvm::PHINode>(user)) {
        MEMOIR_UNREACHABLE("En/decoding uses by PHI is unhandled.");
      } else {
        program_point = user;
      }

    } else {
      MEMOIR_UNREACHABLE("Failed to find a point to encode the value!");
    }

    MemOIRBuilder builder(program_point);

    auto &encoded = add_value(builder, *used);

    uses.value(encoded);

    for (auto *use : uses) {
      update_use(*use, encoded);
    }
  }

  for (auto &uses : decoded) {

    // Unpack the use.
    auto *use = uses.front();
    auto *used = use->get();
    auto *user = dyn_cast<llvm::Instruction>(use->getUser());

    // Compute the insertion point.
    llvm::Instruction *program_point = nullptr;
    if (user) {
      if (auto *phi = dyn_cast<llvm::PHINode>(user)) {
        auto *incoming_block = phi->getIncomingBlock(*use);
        // TODO: this may be unsound if the terminator is conditional.
        program_point = incoming_block->getTerminator();
      } else {
        program_point = user;
      }

    } else {
      MEMOIR_UNREACHABLE("Failed to find a point to decode the value!");
    }

    MemOIRBuilder builder(program_point);

    auto &decoded = decode_value(builder, *used);

    uses.value(decoded);

    for (auto *use : uses) {
      use->set(&decoded);
    }
  }

  return;
}

bool value_will_be_inserted(llvm::Value &value, InsertInst &insert) {

  auto *inst = dyn_cast<llvm::Instruction>(&value);
  llvm::BasicBlock *value_bb = nullptr;
  if (inst) {
    value_bb = inst->getParent();
  } else if (auto *arg = dyn_cast<llvm::Argument>(&value)) {
    auto *func = arg->getParent();
    value_bb = &func->getEntryBlock();
  } else {
    MEMOIR_UNREACHABLE("Unhandled value: ", value);
  }

  auto *user_bb = insert.getParent();

  // If the value and user are in the same basic block, then it will definitely
  // be used.
  if (value_bb == user_bb) {
    return true;
  }

  // If the use postdominates the value definition.
  auto *func = user_bb->getParent();

  // Check if the every path from the value must include the insert instruction,
  // a has instruction equivalent to the inserted key, or is unreachable.
  bool will_be_inserted = true;
  WorkList<llvm::BasicBlock *> worklist = { value_bb };
  while (not worklist.empty()) {
    auto *bb = worklist.pop();

    // If we hit the insert, then this path hits the insert.
    if (bb == user_bb) {
      continue;
    }

    auto *terminator = bb->getTerminator();
    if (isa<llvm::UnreachableInst>(terminator)) {
      // If this path is unreachable, then yippee!
      continue;
    } else if (isa<llvm::ReturnInst>(terminator)) {
      // If we found a return, then we didn't insert along this path!
      will_be_inserted = false;
      break;
    } else if (auto *branch = dyn_cast<llvm::BranchInst>(terminator)) {
      // Check if this branch checks if we have already inserted this value.
      if (auto *cond = branch->getCondition()) {
        if (auto *has = into<HasInst>(cond)) {
          // Check if the offsets match.
          if (&insert.getObject() == &has->getObject()
              and std::equal(insert.indices_begin(),
                             insert.indices_end(),
                             has->indices_begin(),
                             has->indices_end())) {
            // Enqueue the false branch.
            worklist.push(branch->getSuccessor(1));
            continue;
          }
        }
      }

      // Enqueue all successors.
      for (auto *succ : branch->successors()) {
        worklist.push(succ);
      }
    }
  }

  if (will_be_inserted) {
    return true;
  }

  // Otherwise, we can't guarantee the value will be present.
  return false;
}

bool is_total_proxy(ObjectInfo &info, const vector<CoalescedUses> &added) {

  println();
  println("TOTAL PROXY? ", info);

  // Check that the object is not a propagator.
  // NOTE: this is conservative, but the check for a propagator to be a total
  // proxt is difficult.
  if (info.is_propagator()) {
    println("NO, propagator");
    return false;
  }

  // Check that there is guaranteed to be one of these objects for each
  // allocation.
  for (auto offset : info.offsets) {
    if (offset == unsigned(-1)) {
      println("NO, not singular");
      return false;
    }
  }

  // Check that for each addkey use, this object is inserted into.
  // Also, ensure that the encoded value is in a control flow equivalent block
  // to the uses.
  set<llvm::Value *> values_added = {};
  set<llvm::Value *> values_needed = {};
  for (auto &uses : added) {
    auto &value = uses.value();
    auto &encoded =
        MEMOIR_SANITIZE(dyn_cast<llvm::Instruction>(&value),
                        "Encoded value ",
                        value,
                        " is not an instruction! Something went wrong.");

    values_needed.insert(&value);

    bool found_use = false;
    for (auto *use : uses) {
      auto *user = use->getUser();

      if (auto *access = into<AccessInst>(user)) {

        // Skip irrelevant accesses.
        if (not info.is_redefinition(access->getObject())) {
          continue;
        }

        // Check that this access is at the correct offset.
        auto distance = access->match_offsets(info.offsets);
        if (not distance) {
          continue;
        }

        if (distance.value() < info.offsets.size()) {
          println("NO, wrong offset in ", *access);
          return false;
        }

        // Ensure that this access is control equivalent to the encoded value.
        if (auto *insert = dyn_cast<InsertInst>(access)) {
          if (not value_will_be_inserted(value, *insert)) {
            println("NO, not control equivalent");
            return false;
          }
        }

        // Otherwise, we found a valid use.
        found_use = true;

        values_added.insert(&value);

      } else {
        // Skip non-accesses.
        continue;
      }
    }

    // If we failed to find a use, then the check fails.
#if 0
    if (not found_use) {
      println("NO, failed to find use of addkey for: ");
      println("  ", value);
    }
#endif
  }

  // Check if we have added all needed values.
  auto added_all_needed =
      std::accumulate(values_needed.begin(),
                      values_needed.end(),
                      true,
                      [&](bool needed, llvm::Value *val) {
                        return needed and values_added.count(val);
                      });
  if (not added_all_needed) {
    println("NO, did not add all needed values.");
    return false;
  }

  // If we got this far, then we're good to go!
  println("YES!");
  return true;
}

Type &convert_to_sequence_type(Type &base, llvm::ArrayRef<unsigned> offsets) {

  println("CONVERT TO SEQUENCE:");
  println(base);
  for (auto offset : offsets) {
    print(offset, ", ");
  }
  println();

  if (auto *tuple_type = dyn_cast<TupleType>(&base)) {

    vector<Type *> fields = tuple_type->fields();

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
      println("CONVERTED: ", converted);
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

Type &convert_element_type(Type &base,
                           llvm::ArrayRef<unsigned> offsets,
                           Type &new_type) {

  if (auto *tuple_type = dyn_cast<TupleType>(&base)) {

    vector<Type *> fields = tuple_type->fields();

    auto field = offsets[0];

    fields[field] = &convert_element_type(tuple_type->getFieldType(field),
                                          offsets.drop_front(),
                                          new_type);

    return TupleType::get(fields);

  } else if (auto *seq_type = dyn_cast<SequenceType>(&base)) {

    return SequenceType::get(convert_element_type(seq_type->getElementType(),
                                                  offsets.drop_front(),
                                                  new_type),
                             seq_type->get_selection());

  } else if (auto *assoc_type = dyn_cast<AssocType>(&base)) {

    // If the offsets are empty, replace the keys.
    if (offsets.empty()) {

      auto selection = assoc_type->get_selection();
      if (not selection) {
        if (isa<VoidType>(&assoc_type->getValueType())) {
          selection = "bitset";
        } else {
          selection = "bitmap";
        }
      }

      return AssocType::get(new_type, assoc_type->getValueType(), selection);
    }

    return AssocType::get(assoc_type->getKeyType(),
                          convert_element_type(assoc_type->getValueType(),
                                               offsets.drop_front(),
                                               new_type),
                          assoc_type->get_selection());

  } else if (offsets.empty()) {
    return new_type;
  }

  MEMOIR_UNREACHABLE("Failed to convert type!");
}

static void gather_candidate_uses(Candidate &candidate,
                                  set<llvm::Use *> &to_decode,
                                  set<llvm::Use *> &to_encode,
                                  set<llvm::Use *> &to_addkey) {

  map<llvm::Function *, set<llvm::Value *>> encoded = {};
  for (auto *info : candidate) {
    for (const auto &[func, values] : info->encoded) {
      encoded[func].insert(values.begin(), values.end());
    }

    for (const auto &[func, uses] : info->to_encode) {
      to_encode.insert(uses.begin(), uses.end());
    }

    for (const auto &[func, uses] : info->to_addkey) {
      to_addkey.insert(uses.begin(), uses.end());
    }
  }

  // Perform a forward analysis on the encoded values.
  forward_analysis(encoded);

  // Collect the set of uses to decode.
  for (const auto &[func, values] : encoded) {
    for (auto *val : values) {
      for (auto &use : val->uses()) {
        auto *user = use.getUser();

        // If the user is a PHI/Select/Fold _and_ is also encoded, we don't
        // need to decode it.
        if (isa<llvm::PHINode>(user) or isa<llvm::SelectInst>(user)) {
          if (encoded[func].count(user) > 0) {
            continue;
          }
        } else if (auto *fold = into<FoldInst>(user)) {
          auto *arg = (use == fold->getInitialAsUse())
                          ? &fold->getAccumulatorArgument()
                          : fold->getClosedArgument(use);
          if (encoded[&fold->getBody()].count(arg)) {
            continue;
          }
        } else if (auto *call = dyn_cast<llvm::CallBase>(user)) {
          // TODO
        }

        // If the user is not an encoded propagator, we need to decode this use.
        to_decode.insert(&use);
      }
    }
  }

  return;
}

bool ProxyInsertion::transform() {

  bool modified = false;

  // Collect the set of redefinitions for each allocation involved.
  for (auto candidates_it = this->candidates.begin();
       candidates_it != this->candidates.end();
       ++candidates_it) {

    modified |= true;

    auto &candidate = *candidates_it;

    infoln();
    infoln("PROXYING CANDIDATE:");
    for (const auto *candidate_info : candidate) {
      infoln("  ", *candidate_info);
    }

    // Unpack the first object information.
    auto *first_info = candidate.front();
    auto *alloc = first_info->allocation;
    auto *type = &alloc->getType();

    // Get the nested object type.
    for (auto offset : first_info->offsets) {
      if (auto *struct_type = dyn_cast<TupleType>(type)) {
        type = &struct_type->getFieldType(offset);
      } else if (auto *collection_type = dyn_cast<CollectionType>(type)) {
        type = &collection_type->getElementType();
      } else {
        MEMOIR_UNREACHABLE("Invalid offsets provided.");
      }
    }

    auto &assoc_type = MEMOIR_SANITIZE(
        dyn_cast<AssocType>(type),
        "Proxy insertion for non-assoc collection is unsupported");

    auto &key_type = assoc_type.getKeyType();
    auto &val_type = assoc_type.getValueType();

    // Collect all of the uses that need to be handled.
    set<llvm::Use *> to_decode = {};
    set<llvm::Use *> to_encode = {};
    set<llvm::Use *> to_addkey = {};
    gather_candidate_uses(candidate, to_decode, to_encode, to_addkey);

    println();
    println("  before trimming:");
    println("  ", to_encode.size(), " uses to encode");
    for (auto *use : to_encode) {
      infoln(pretty_use(*use));
    }
    println();
    println("  ", to_decode.size(), " uses to decode");
    for (auto *use : to_decode) {
      infoln(pretty_use(*use));
    }
    println();
    println("  ", to_addkey.size(), " uses to addkey");
    for (auto *use : to_addkey) {
      infoln(pretty_use(*use));
    }

    // Trim uses that dont need to be decoded because they are only used to
    // compare against other values that need to be decoded.
    set<llvm::Use *> trim_to_decode = {};
    for (auto *use : to_decode) {
      auto *user = use->getUser();

      if (auto *cmp = dyn_cast<llvm::CmpInst>(user)) {
        if (cmp->isEquality()) {
          auto &lhs = cmp->getOperandUse(0);
          auto &rhs = cmp->getOperandUse(1);
          if (used_value_will_be_decoded(lhs, to_decode)
              and used_value_will_be_decoded(rhs, to_decode)) {
            trim_to_decode.insert(&lhs);
            trim_to_decode.insert(&rhs);
          }
        }
      } else if (auto *phi = dyn_cast<llvm::PHINode>(user)) {
        bool all_decode = true;
        for (auto &incoming : phi->incoming_values()) {
          if (not used_value_will_be_decoded(incoming, to_decode)) {
            all_decode = false;
            break;
          }
        }
        if (all_decode) {
          for (auto &incoming : phi->incoming_values()) {
            trim_to_decode.insert(&incoming);
          }
        }
      }
    }

    // Trim uses that dont need to be encoded because they are produced by a
    // use that needs decoded.
    set<llvm::Use *> trim_to_encode = {};
    for (auto uses : { to_encode, to_addkey }) {
      for (auto *use : uses) {
        if (used_value_will_be_decoded(*use, to_decode)) {
          trim_to_encode.insert(use);
          trim_to_decode.insert(use);
        }
      }
    }

    // Erase the uses that we identified to trim.
    for (auto *use_to_trim : trim_to_decode) {
      to_decode.erase(use_to_trim);
    }
    for (auto *use_to_trim : trim_to_encode) {
      to_encode.erase(use_to_trim);
      to_addkey.erase(use_to_trim);
    }

    {
      println("  trimmed:");
      println("  ", trim_to_encode.size(), " uses to encode");
      for (auto *use : trim_to_encode) {
        infoln(pretty_use(*use));
      }

      println("  ", trim_to_decode.size(), " uses to decode");
      for (auto *use : trim_to_decode) {
        infoln(pretty_use(*use));
      }
    }

    {
      println("  after trimming:");
      println("  ", to_encode.size(), " uses to encode");
      for (auto *use : to_encode) {
        infoln(pretty_use(*use));
      }

      println("  ", to_decode.size(), " uses to decode");
      for (auto *use : to_decode) {
        infoln(pretty_use(*use));
      }

      println("  ", to_addkey.size(), " uses to addkey");
      for (auto *use : to_addkey) {
        infoln(pretty_use(*use));
      }
    }

    // Find the construction point for the encoder and decoder.
    llvm::Instruction *construction_point = &alloc->getCallInst();

    // Find a point that dominates all of the object allocations.
    auto *construction_function = construction_point->getFunction();
    auto &DT = this->get_dominator_tree(*construction_function);
    for (const auto *other : candidate) {
      auto *other_alloc = other->allocation;
      auto &other_inst = other_alloc->getCallInst();

      construction_point =
          DT.findNearestCommonDominator(construction_point, &other_inst);
    }

    // Fetch LLVM context.
    auto &context = construction_point->getContext();
    auto &module = MEMOIR_SANITIZE(construction_point->getModule(),
                                   "Construction point has no module.");
    auto &data_layout = module.getDataLayout();

    // Allocate the proxy.
    MemOIRBuilder builder(construction_point);

    auto &size_type = Type::get_size_type(data_layout);
    auto &llvm_size_type = *size_type.get_llvm_type(context);
    auto size_type_bitwidth = size_type.getBitWidth();

    // Determine which proxies we need.
    bool build_encoder = to_encode.size() > 0 or to_addkey.size() > 0;
    bool build_decoder = to_decode.size() > 0;

    llvm::Instruction *encoder = nullptr;
    Type *encoder_type = nullptr;
    if (build_encoder) {
      encoder_type = &AssocType::get(key_type, size_type);
      auto *encoder_alloc =
          builder.CreateAllocInst(*encoder_type, {}, "proxy.encode.");
      encoder = &encoder_alloc->getCallInst();
    }

    llvm::Instruction *decoder = nullptr;
    Type *decoder_type = nullptr;
    if (build_decoder) {
      decoder_type = &SequenceType::get(key_type);
      auto *decoder_alloc = builder.CreateAllocInst(*decoder_type,
                                                    { builder.getInt64(0) },
                                                    "proxy.decode.");
      decoder = &decoder_alloc->getCallInst();
    }

    // Make the proxy available at all uses.
    map<llvm::Function *, llvm::Instruction *> function_to_encoder = {};
    if (build_encoder) {
      add_tempargs(function_to_encoder,
                   { to_encode, to_addkey },
                   *encoder,
                   *encoder_type,
                   "encoder.");
    }

    map<llvm::Function *, llvm::Instruction *> function_to_decoder = {};
    if (build_decoder) {
      add_tempargs(function_to_decoder,
                   { to_decode, to_addkey },
                   *decoder,
                   *decoder_type,
                   "decoder.");
    }

    // Create the addkey function.
    auto addkey_callee = create_addkey_function(this->M,
                                                key_type,
                                                build_encoder,
                                                encoder_type,
                                                build_decoder,
                                                decoder_type);

    // Coalesce the values that have been encoded/decoded.
    vector<CoalescedUses> decoded = {};
    vector<CoalescedUses> encoded = {};
    vector<CoalescedUses> added = {};
    coalesce(decoded,
             encoded,
             added,
             to_decode,
             to_encode,
             to_addkey,
             this->get_dominator_tree);

    // Report the coalescing.
    println("AFTER COALESCING:");
    println(encoded.size(), " uses to encode");
    println(decoded.size(), " uses to decode");
    println(added.size(), " uses to addkey");

    // Create anon functions to encode/decode a value
    std::function<llvm::Value &(llvm::Value &)> get_encoder =
        [&](llvm::Value &value) -> llvm::Value & {
      auto *function = parent_function(value);
      MEMOIR_ASSERT(function, "Failed to find parent function for ", value);
      return MEMOIR_SANITIZE(function_to_encoder[function],
                             "Failed to find encoder in ",
                             function->getName());
    };

    std::function<llvm::Value &(MemOIRBuilder &, llvm::Value &)> decode_value =
        [&](MemOIRBuilder &builder, llvm::Value &value) -> llvm::Value & {
      auto *function = parent_function(value);
      MEMOIR_ASSERT(function, "Failed to find parent function for ", value);
      auto *decoder = function_to_decoder.at(function);
      return builder.CreateReadInst(key_type, decoder, { &value })->asValue();
    };

    std::function<llvm::Value &(MemOIRBuilder &, llvm::Value &)> encode_value =
        [&](MemOIRBuilder &builder, llvm::Value &value) -> llvm::Value & {
      auto *function = parent_function(value);
      MEMOIR_ASSERT(function, "Failed to find parent function for ", value);
      auto *encoder = function_to_encoder.at(function);
      return builder.CreateReadInst(size_type, encoder, &value)->asValue();
    };

    std::function<llvm::Value &(MemOIRBuilder &, llvm::Value &)> add_value =
        [&](MemOIRBuilder &builder, llvm::Value &value) -> llvm::Value & {
      auto *function = parent_function(value);
      MEMOIR_ASSERT(function, "Failed to find parent function for ", value);

      vector<llvm::Value *> args = { &value };
      if (build_encoder) {
        auto *encoder = function_to_encoder.at(function);
        args.push_back(encoder);
      }
      if (build_decoder) {
        auto *decoder = function_to_decoder.at(function);
        args.push_back(decoder);
      }

      return MEMOIR_SANITIZE(builder.CreateCall(addkey_callee, args),
                             "Failed to create call to addkey!");
    };

    // Inject instructions to handle each use.
    inject(context,
           decoded,
           encoded,
           added,
           get_encoder,
           decode_value,
           encode_value,
           add_value);

    // Collect the types that each candidate needs to be mutated to.
    AssocList<AllocInst *, Type *> types_to_mutate;
    for (auto *info : candidate) {
      auto *alloc = info->allocation;

      // Initialize the type to mutate if it doesnt exist already.
      auto found = types_to_mutate.find(alloc);
      if (found == types_to_mutate.end()) {
        types_to_mutate[alloc] = &alloc->getType();
      }
      auto &type = types_to_mutate[alloc];

      // If the object is a total proxy, update it to be a sequence.
      if (is_total_proxy(*info, added)) {

        println(Style::BOLD, Colors::GREEN, "FOUND TOTAL PROXY", Style::RESET);

        type = &convert_to_sequence_type(*type, info->offsets);

      } else {
        // Convert the type at the given offset to the size type.
        auto &size_type =
            Type::get_size_type(alloc->getModule()->getDataLayout());

        type = &convert_element_type(*type, info->offsets, size_type);
      }
    }

    // Mutate the types in the program.
    auto mutate_it = types_to_mutate.begin(), mutate_ie = types_to_mutate.end();
    for (; mutate_it != mutate_ie; ++mutate_it) {
      auto *alloc = mutate_it->first;
      auto *type = mutate_it->second;

      // Mutate the type.
      mutate_type(
          *alloc,
          *type,
          [&](llvm::Function &old_func,
              llvm::Function &new_func,
              llvm::ValueToValueMapTy &vmap) {
            // Update the remaining candidate.
            for (auto it = std::next(candidates_it); it != candidates.end();
                 ++it) {
              for (auto &info : *it) {
                info->update(old_func,
                             new_func,
                             vmap,
                             /* delete old? */ true);
              }
            }

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
  }

  return modified;
}

ProxyInsertion::ProxyInsertion(llvm::Module &M,
                               GetDominatorTree get_dominator_tree)
  : M(M),
    get_dominator_tree(get_dominator_tree) {

  // Register the bit{map,set} implementations.
  Implementation::define({
      Implementation( // bitset<T>
          "bitset",
          AssocType::get(TypeVariable::get(), VoidType::get())),

      Implementation( // bitmap<T, U>
          "bitmap",
          AssocType::get(TypeVariable::get(), TypeVariable::get())),

  });

  analyze();

  transform();

  for (auto &F : M) {
    if (not F.empty()) {
      if (llvm::verifyFunction(F, &llvm::errs())) {
        println(F);
        MEMOIR_UNREACHABLE("Failed to verify ", F.getName());
      }
    }
  }
  println("Verified module post-proxy insertion.");

  MemOIRInst::invalidate();

  reify_tempargs(M);

  for (auto &F : M) {
    if (not F.empty()) {
      if (llvm::verifyFunction(F, &llvm::errs())) {
        println(F);
        MEMOIR_UNREACHABLE("Failed to verify ", F.getName());
      }
    }
  }
  println("Verified module post-temparg reify.");
}

} // namespace folio
