#include "folio/ProxyInsertion.hpp"
#include "folio/Utilities.hpp"

using namespace llvm::memoir;

namespace folio {

// ================
static void update_values(llvm::ValueToValueMapTy &vmap,
                          const Set<llvm::Value *> &input,
                          Set<llvm::Value *> &output) {
  for (auto *val : input) {
    auto *clone = &*vmap[val];

    output.insert(clone);
  }
}

static void update_values(llvm::ValueToValueMapTy &vmap,
                          const Set<NestedObject> &input,
                          Set<NestedObject> &output) {
  for (const auto &info : input) {
    auto *clone = &*vmap[&info.value()];

    output.emplace(*clone, info.offsets());
  }
}

template <typename T>
static void update_values(llvm::ValueToValueMapTy &vmap,
                          const LocalMap<T> &input,
                          LocalMap<T> &output) {
  for (const auto &[base, redefs] : input) {
    auto *clone = &*vmap[base];
    update_values(vmap, redefs, output[clone]);
  }
}

static void update_uses(Map<llvm::Function *, Set<llvm::Use *>> &uses,
                        llvm::Function &old_func,
                        llvm::Function &new_func,
                        llvm::ValueToValueMapTy &vmap,
                        bool delete_old) {

  if (uses.count(&old_func)) {

    auto &old_uses = uses[&old_func];
    auto &new_uses = uses[&new_func];

    for (auto *use : old_uses) {
      auto *user = dyn_cast<llvm::Instruction>(use->getUser());
      auto *clone = dyn_cast<llvm::Instruction>(&*vmap[user]);

      auto &clone_use = clone->getOperandUse(use->getOperandNo());

      new_uses.insert(&clone_use);
    }

    if (delete_old) {
      uses.erase(&old_func);
    }
  }
}

static llvm::Use *remap(llvm::Use *use, llvm::ValueToValueMapTy &vmap) {
  auto *user = use->getUser();
  auto *new_user = cast<llvm::User>(&*vmap[user]);
  return &new_user->getOperandUse(use->getOperandNo());
}

static llvm::Value *remap(llvm::Value *val, llvm::ValueToValueMapTy &vmap) {
  return &*vmap[val];
}

template <typename T>
static void update_bases(Map<T *, llvm::Value *> bases,
                         llvm::Function &old_func,
                         llvm::Function &new_func,
                         llvm::ValueToValueMapTy &vmap,
                         bool delete_old) {

  Map<T *, llvm::Value *> new_bases = {};
  for (auto it = bases.begin(); it != bases.end();) {
    auto [val, base] = *it;
    auto *func = parent_function(*val);

    if (func != &old_func) {
      continue;
    }

    new_bases[remap(val, vmap)] = remap(base, vmap);

    if (delete_old) {
      it = bases.erase(it);
    } else {
      ++it;
    }
  }

  for (const auto &[val, base] : new_bases) {
    bases[val] = base;
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

    for (auto [it, ie] = redefs.equal_range(&old_func); it != ie; ++it) {
      const auto &[func, call_to_locals] = *it;
      const auto &[call, old_locals] = call_to_locals;

      // Insert the context into the redefs.
      auto &new_locals = redefs.emplace(&new_func, call);

      update_values(vmap, old_locals, new_locals);
    }

    if (delete_old) {
      redefs.erase(&old_func);
    }
  }

  // Update the uses.
  update_uses(this->to_encode, old_func, new_func, vmap, delete_old);
  update_uses(this->to_addkey, old_func, new_func, vmap, delete_old);

  // Update the set of encoded values.
  update_values(vmap, this->encoded[&old_func], this->encoded[&new_func]);

  // Update the mappings to base.
  update_bases(encoded_base, old_func, new_func, vmap, delete_old);
  update_bases(to_encode_base, old_func, new_func, vmap, delete_old);
  update_bases(to_addkey_base, old_func, new_func, vmap, delete_old);

  if (delete_old) {
    this->encoded[&old_func].clear();
  }

  return;
}

uint32_t ObjectInfo::compute_heuristic(const ObjectInfo &other) const {
  uint32_t benefit = 0;

  // Merge the encoded values of both and perform a forward analysis.
  Map<llvm::Function *, Set<llvm::Value *>> merged = {};
  for (const auto &[func, values] : this->encoded) {
    merged[func].insert(values.begin(), values.end());
  }
  for (const auto &[func, values] : other.encoded) {
    merged[func].insert(values.begin(), values.end());
  }

  forward_analysis(merged);

  for (const auto &[func, encoded] : merged) {
    for (const auto *value : encoded) {
      for (auto &use_to_decode : value->uses()) {

        auto *user = use_to_decode.getUser();

        if (other.to_encode.count(func)) {
          for (const auto *use_to_encode : other.to_encode.at(func)) {
            if (&use_to_decode == use_to_encode) {
              ++benefit;
            }
          }
        }

        if (other.to_addkey.count(func)) {
          for (const auto *use_to_addkey : other.to_addkey.at(func)) {
            if (&use_to_decode == use_to_addkey) {
              ++benefit;
            }
          }
        }

        if (other.encoded.count(func)) {
          auto *cmp = dyn_cast<llvm::CmpInst>(user);
          if (cmp and cmp->isEquality()) {
            if (encoded.count(cmp->getOperand(0))) {
              ++benefit;
            } else if (encoded.count(cmp->getOperand(1))) {
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
  for (const auto &[func, base_to_redefs] : this->redefinitions) {
    for (const auto &[base, redefs] : base_to_redefs.second) {
      for (const auto &redef : redefs) {
        if (&redef.value() == &V) {
          return true;
        }
      }
    }
  }

  return false;
}

static void gather_redefinitions(const NestedObject &obj,
                                 ObjectInfo::Redefinitions &redefinitions,
                                 const NestedObject &base,
                                 llvm::CallBase *context = NULL) {

  auto *function = parent_function(obj.value());
  MEMOIR_ASSERT(function, "Unknown parent function for redefinition.");

  infoln("GATHER ", obj);

  // Fetch the set of local redefinitions to update.
  auto &local_redefs = redefinitions.emplace(function, context)[&base.value()];

  // If we've already visited this object, return.
  if (local_redefs.contains(obj)) {
    return;
  } else {
    local_redefs.insert(obj);
  }

  // Iterate over all uses of this object to find redefinitions.
  for (auto &use : obj.value().uses()) {
    auto *user = use.getUser();

    NestedObject user_obj(*user, obj.offsets());

    // Recurse on redefinitions.
    if (auto *phi = dyn_cast<llvm::PHINode>(user)) {
      gather_redefinitions(user_obj, redefinitions, base, context);
    } else if (auto *memoir_inst = into<MemOIRInst>(user)) {
      if (auto *update = dyn_cast<UpdateInst>(memoir_inst)) {
        if (&use == &update->getObjectAsUse()) {
          gather_redefinitions(user_obj, redefinitions, base, context);
        }

      } else if (isa<RetPHIInst>(memoir_inst) or isa<UsePHIInst>(memoir_inst)) {

        // Recurse on redefinitions.
        gather_redefinitions(user_obj, redefinitions, base, context);
      }

      // Gather variable if folded on, or recurse on closed argument.
      else if (auto *fold = into<FoldInst>(user)) {

        if (&use == &fold->getInitialAsUse()) {
          // Gather uses of the accumulator argument.
          auto &accum = fold->getAccumulatorArgument();
          gather_redefinitions(NestedObject(accum, obj.offsets()),
                               redefinitions,
                               NestedObject(accum, base.offsets()),
                               // Propagate the context, since the fold body is
                               // considered to be _in_ this function.
                               context);

          // Gather uses of the resultant.
          auto &result = fold->getResult();
          gather_redefinitions(NestedObject(result, obj.offsets()),
                               redefinitions,
                               base,
                               context);

        } else if (&use == &fold->getObjectAsUse()) {

          // If the element argument is an object, gather uses of it.
          auto *elem_arg = fold->getElementArgument();
          if (elem_arg and Type::value_is_object(*elem_arg)) {

            // Try to match the access indices against the offsets.
            auto maybe_distance = fold->match_offsets(base.offsets());
            if (not maybe_distance) {
              continue;
            }
            auto distance = maybe_distance.value();

            // If the offsets are not fully exhausted, recurse on the value
            // argument.
            if (distance < base.offsets().size()) {

              // Construct the nested offsets.
              Vector<unsigned> nested_offsets(obj.offsets().begin(),
                                              obj.offsets().end());

              // Append the new offsets from the base.
              auto new_offset = base.offsets().take_front(distance + 1);
              nested_offsets.insert(nested_offsets.end(),
                                    new_offset.begin(),
                                    new_offset.end());

              // Recurse.
              gather_redefinitions(NestedObject(*elem_arg, nested_offsets),
                                   redefinitions,
                                   NestedObject(*elem_arg, base.offsets()),
                                   // Propagate the context, since the fold body
                                   // is considered to be _in_ this function.
                                   context);
            }
          }

        } else if (auto *closed_arg = fold->getClosedArgument(use)) {
          // Gather uses of the closed argument.
          gather_redefinitions(NestedObject(*closed_arg, obj.offsets()),
                               redefinitions,
                               NestedObject(*closed_arg, base.offsets()),
                               // Propagate the context, since the fold body is
                               // considered to be _in_ this function.
                               context);
        }
      }
    } else if (auto *call = dyn_cast<llvm::CallBase>(user)) {
      auto &callee =
          MEMOIR_SANITIZE(call->getCalledFunction(),
                          "Found use of MEMOIR collection by indirect callee!");

      auto &arg =
          MEMOIR_SANITIZE(callee.getArg(use.getOperandNo()),
                          "No arguments in the callee matching this use!");

      gather_redefinitions(NestedObject(arg, obj.offsets()),
                           redefinitions,
                           NestedObject(arg, base.offsets()),
                           call);
    }
  }

  return;
}

static void gather_redefinitions(ObjectInfo::Redefinitions &redefinitions,
                                 llvm::Value &value,
                                 llvm::ArrayRef<unsigned> base_offset) {
  gather_redefinitions(NestedObject(value),
                       redefinitions,
                       NestedObject(value, base_offset));
}

static llvm::Use *get_use_at_offset(llvm::User::op_iterator index_begin,
                                    llvm::User::op_iterator index_end,
                                    llvm::ArrayRef<unsigned> offsets) {
  auto index_it = index_begin, index_ie = index_end;

  for (auto offset : offsets) {

    // If we have reached the end of the index operands, there is no index
    // use.
    if (index_it == index_ie) {
      return nullptr;
    }

    // If the offset is not a placeholder, make sure it matches.
    if (offset != -1) {
      auto &index_use = *index_it;
      auto &index_const =
          MEMOIR_SANITIZE(dyn_cast<llvm::ConstantInt>(index_use.get()),
                          "Field index is not statically known!");

      // If the offset doesn't match the field index, there is no index use.
      if (offset != index_const.getZExtValue()) {
        return nullptr;
      }
    }

    ++index_it;
  }

  // If we are at the end of the index operands, return NULL.
  if (index_it == index_ie) {
    return nullptr;
  }

  // Otherwise, return the current use.
  return &*index_it;
}

static llvm::Use *get_index_use(AccessInst &access,
                                llvm::ArrayRef<unsigned> offsets) {
  return get_use_at_offset(access.index_operands_begin(),
                           access.index_operands_end(),
                           offsets);
}

static void gather_uses_to_proxy(
    llvm::Value &V,
    llvm::ArrayRef<unsigned> offsets,
    Map<llvm::Function *, Set<llvm::Value *>> &encoded,
    Map<llvm::Function *, Set<llvm::Use *>> &to_encode,
    Map<llvm::Function *, Set<llvm::Use *>> &to_addkey,
    Set<llvm::Value *> &base_encoded,
    Set<llvm::Use *> &base_to_encode,
    Set<llvm::Use *> &base_to_addkey) {

#define DECODE(FUNC, VAL)                                                      \
  {                                                                            \
    encoded[FUNC].insert(VAL);                                                 \
    base_encoded.insert(VAL);                                                  \
  }
#define ENCODE(FUNC, USE)                                                      \
  {                                                                            \
    to_encode[FUNC].insert(USE);                                               \
    base_to_encode.insert(USE);                                                \
  }
#define ADDKEY(FUNC, USE)                                                      \
  {                                                                            \
    to_addkey[FUNC].insert(USE);                                               \
    base_to_addkey.insert(USE);                                                \
  }

  infoln("REDEF ", V, " IN ", parent_function(V)->getName());

  auto *function = parent_function(V);
  MEMOIR_ASSERT(function, "Gathering uses of value with no parent function!");

  // From a given collection, V, gather all uses that need to be either
  // encoded or decoded.
  for (auto &use : V.uses()) {
    auto *user = use.getUser();

    infoln("  USER ", *user);

    // We only need to handle acceses.
    if (auto *access = into<AccessInst>(user)) {

      // Check if any of the index uses need to be updated.
      if (&use == &access->getObjectAsUse()) {

        // Check that the access matches the offsets.
        auto maybe_distance = access->match_offsets(offsets);
        if (not maybe_distance) {
          continue; // Do nothing.
        }
        auto distance = maybe_distance.value();

        // If we find the index to handle in the indices list, then mark it for
        // encoding and continue;.
        if (auto *index_use = get_index_use(*access, offsets)) {
          if (isa<InsertInst>(access)) {
            if (is_last_index(index_use, access->index_operands_end())) {
              infoln("    ADDING KEY ", *index_use->get());
              ADDKEY(function, index_use);
              continue;
            }
          }

          infoln("    ENCODING KEY ", *index_use->get());
          ENCODE(function, index_use);
          continue;
        }

        // Handle fold operations specially for recursion.
        else if (auto *fold = dyn_cast<FoldInst>(access)) {
          // If the offsets are fully exhausted, add uses of the index
          // argument to the set of uses to decode.
          if (distance == offsets.size()) {
            auto &index_arg = fold->getIndexArgument();
            infoln("    DECODING KEY ",
                   index_arg,
                   " IN ",
                   fold->getBody().getName());
            DECODE(&fold->getBody(), &index_arg);
            continue;
          }
        }

      } else if (auto input_kw = access->get_keyword<InputKeyword>()) {
        if (&input_kw->getInputAsUse() == &use) {
          auto &type =
              MEMOIR_SANITIZE(type_of(V),
                              "Failed to get type of object used as input.");
          if (auto *index_use = get_use_at_offset(input_kw->index_ops_begin(),
                                                  input_kw->index_ops_end(),
                                                  offsets)) {
            infoln("    ENCODING KEY ", *index_use->get());
            ENCODE(function, index_use);
            continue;
          }
        }
      }
    }
  }

  return;
}

static void gather_uses_to_propagate(
    llvm::Value &V,
    llvm::ArrayRef<unsigned> offsets,
    Map<llvm::Function *, Set<llvm::Value *>> &encoded,
    Map<llvm::Function *, Set<llvm::Use *>> &to_encode,
    Map<llvm::Function *, Set<llvm::Use *>> &to_addkey,
    Set<llvm::Value *> &base_encoded,
    Set<llvm::Use *> &base_to_encode,
    Set<llvm::Use *> &base_to_addkey) {

#define DECODE(FUNC, VAL)                                                      \
  {                                                                            \
    encoded[FUNC].insert(VAL);                                                 \
    base_encoded.insert(VAL);                                                  \
  }
#define ENCODE(FUNC, USE)                                                      \
  {                                                                            \
    to_encode[FUNC].insert(USE);                                               \
    base_to_encode.insert(USE);                                                \
  }
#define ADDKEY(FUNC, USE)                                                      \
  {                                                                            \
    to_addkey[FUNC].insert(USE);                                               \
    base_to_addkey.insert(USE);                                                \
  }

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
      if (&use != &access->getObjectAsUse()) {
        infoln("    Not object use");
        continue;
      }

      // Try to match the access indices against the offsets.
      auto maybe_distance = access->match_offsets(offsets);

      // If the indices don't match, skip.
      if (not maybe_distance) {
        infoln("    Offsets don't match");
        continue;
      }

      auto distance = maybe_distance.value();

      if (auto *fold = dyn_cast<FoldInst>(access)) {
        // If the offsets are fully exhausted, add uses of the index
        // argument to the set of uses to decode.
        // NOTE: because the offsets match the elements of the collection, we
        // need to do distance+1 here.
        if ((distance + 1) == offsets.size()) {
          if (auto *elem_arg = fold->getElementArgument()) {
            infoln("    DECODING ELEM");
            DECODE(&fold->getBody(), elem_arg);
          }
        }

      } else if (auto *read = dyn_cast<ReadInst>(access)) {
        if (distance == offsets.size()) {
          infoln("    DECODING ELEM ");
          auto &value = read->asValue();
          DECODE(function, &value)
        }

      } else if (auto *update = dyn_cast<UpdateInst>(access)) {
        // Handle values.
        if (auto *write = dyn_cast<WriteInst>(access)) {
          if (distance == offsets.size()) {
            infoln("    ADDKEY ");
            auto &val_use = write->getValueWrittenAsUse();
            ADDKEY(function, &val_use);
          }

        } else if (auto *insert = dyn_cast<InsertInst>(access)) {
          if (auto value_kw = insert->get_keyword<ValueKeyword>()) {
            if (distance == offsets.size()) {
              infoln("    ADDKEY ");
              auto &val_use = value_kw->getValueAsUse();
              ADDKEY(function, &val_use);
            }
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

template <typename T>
static void populate_reverse(const LocalMap<Set<T>> &input,
                             Map<T, llvm::Value *> &output) {
  // Create the reverse mappings.
  for (const auto &[base, values] : input) {
    for (auto *val : values) {
      auto found = output.find(val);
      if (found == output.end()) {
        // Not found, insert.
        output.emplace_hint(found, val, base);
        continue;
      }

      if (base == found->second) {
        // Found, no conflict.
        continue;
      }

      // Otherwise, we found the value and it has a conflict!
      MEMOIR_UNREACHABLE("Multiple bases found for ",
                         pretty(*val),
                         "\n  IN ",
                         parent_function(*val)->getName(),
                         "\n  BASE  ",
                         pretty(*base),
                         "\n  FOUND ",
                         pretty(*found->second));
    }
  }
}

void ObjectInfo::analyze() {
  println();
  println("ANALYZING ", *this);

  auto &alloc = this->allocation->asValue();

  gather_redefinitions(this->redefinitions, alloc, this->offsets);

  bool is_propagator = this->is_propagator();

  println("PROPAGATOR? ", is_propagator ? "YES" : "NO");

  llvm::ArrayRef<unsigned> offsets(this->offsets);

  LocalMap<Set<llvm::Value *>> base_encoded;
  LocalMap<Set<llvm::Use *>> base_to_encode, base_to_addkey;

  for (const auto &[func, base_to_redefs] : this->redefinitions) {
    for (const auto &[base, redefs] : base_to_redefs.second) {
      infoln("REDEFS(", *base, ")");

      for (const auto &redef : redefs) {
        infoln("  ", redef);

        if (is_propagator) {
          gather_uses_to_propagate(redef.value(),
                                   offsets.drop_front(redef.offsets().size()),
                                   this->encoded,
                                   this->to_encode,
                                   this->to_addkey,
                                   base_encoded[base],
                                   base_to_encode[base],
                                   base_to_addkey[base]);
        } else {
          gather_uses_to_proxy(redef.value(),
                               offsets.drop_front(redef.offsets().size()),
                               this->encoded,
                               this->to_encode,
                               this->to_addkey,
                               base_encoded[base],
                               base_to_encode[base],
                               base_to_addkey[base]);
        }
      }
    }
  }

  // Populate the reverse mappings.
  populate_reverse(base_encoded, this->encoded_base);
  populate_reverse(base_to_encode, this->to_encode_base);
  populate_reverse(base_to_addkey, this->to_addkey_base);
}

Type &ObjectInfo::get_type() const {
  auto *type = &this->allocation->getType();

  for (auto offset : this->offsets) {
    if (auto *tuple_type = dyn_cast<TupleType>(type)) {
      type = &tuple_type->getFieldType(offset);
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

bool operator<(const NestedObject &lhs, const NestedObject &rhs) {
  auto *lvalue = &lhs.value(), *rvalue = &rhs.value();
  if (lvalue != rvalue) {
    return lvalue < rvalue;
  }

  auto lsize = lhs.offsets().size(), rsize = rhs.offsets().size();
  if (lsize != rsize) {
    return lsize < rsize;
  }

  for (size_t i = 0; i < lsize; ++i) {
    auto loffset = lhs.offsets()[i], roffset = rhs.offsets()[i];
    if (loffset != roffset) {
      return loffset < roffset;
    }
  }

  return false;
}

bool operator==(const NestedObject &lhs, const NestedObject &rhs) {
  auto *lvalue = &lhs.value(), *rvalue = &rhs.value();
  if (lvalue != rvalue) {
    return false;
  }

  auto lsize = lhs.offsets().size(), rsize = rhs.offsets().size();
  if (lsize != rsize) {
    return false;
  }

  for (size_t i = 0; i < lsize; ++i) {
    auto loffset = lhs.offsets()[i], roffset = rhs.offsets()[i];
    if (loffset != roffset) {
      return false;
    }
  }

  return true;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const NestedObject &obj) {
  os << "(" << obj.value() << ")";
  for (auto offset : obj.offsets()) {
    if (offset == unsigned(-1)) {
      os << "[*]";
    } else {
      os << "." << std::to_string(offset);
    }
  }

  return os;
}

} // namespace folio
