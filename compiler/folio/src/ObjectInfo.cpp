#include "folio/ProxyInsertion.hpp"
#include "folio/Utilities.hpp"

using namespace llvm::memoir;

namespace folio {

// Private accessors
Map<llvm::Function *, LocalInfo> &ObjectInfo::info() {
  return this->_info;
}

const Map<llvm::Function *, LocalInfo> &ObjectInfo::info() const {
  return this->_info;
}

void ObjectInfo::decode(llvm::Function &function, llvm::Value &value) {
  this->encoded(function).insert(&value);
}

void ObjectInfo::encode(llvm::Function &function, llvm::Use &use) {
  this->to_encode(function).insert(&use);
}

void ObjectInfo::addkey(llvm::Function &function, llvm::Use &use) {
  this->to_addkey(function).insert(&use);
}

// Accessors
const ObjectInfoKind &ObjectInfo::kind() const {
  return this->_kind;
}

LocalInfo &ObjectInfo::local(llvm::Function &function) {
  return this->_info[&function];
}

const LocalInfo &ObjectInfo::local(llvm::Function &function) const {
  return this->_info.at(&function);
}

Set<Object> &ObjectInfo::redefinitions(llvm::Function &function) {
  return this->local(function).redefinitions;
}

const Set<Object> &ObjectInfo::redefinitions(llvm::Function &function) const {
  return this->local(function).redefinitions;
}

Set<llvm::Value *> &ObjectInfo::encoded(llvm::Function &function) {
  return this->local(function).encoded;
}

const Set<llvm::Value *> &ObjectInfo::encoded(llvm::Function &function) const {
  return this->local(function).encoded;
}

Set<llvm::Use *> &ObjectInfo::to_encode(llvm::Function &function) {
  return this->local(function).to_encode;
}

const Set<llvm::Use *> &ObjectInfo::to_encode(llvm::Function &function) const {
  return this->local(function).to_encode;
}

Set<llvm::Use *> &ObjectInfo::to_addkey(llvm::Function &function) {
  return this->local(function).to_addkey;
}

const Set<llvm::Use *> &ObjectInfo::to_addkey(llvm::Function &function) const {
  return this->local(function).to_addkey;
}

bool ObjectInfo::is_propagator() const {
  return not isa<CollectionType>(&this->type());
}

bool ObjectInfo::is_redefinition(llvm::Value &value) const {
  auto *function = parent<llvm::Function>(value);
  if (this->info().contains(function))
    for (const auto &obj : this->redefinitions(*function))
      if (&value == &obj.value())
        return true;

  return false;
}

void ObjectInfo::gather_redefinitions() {
  return this->gather_redefinitions(*this);
}

void ObjectInfo::gather_redefinitions(const Object &obj) {

  println("GATHER ", obj);

  // Get the function parent.
  auto &function = MEMOIR_SANITIZE(this->function(),
                                   "Unknown parent function for redefinition.");

  // Fetch the set of local redefinitions to update.
  auto &local_redefs = this->redefinitions(function);

  // If we've already visited this object, return.
  if (local_redefs.contains(obj))
    return;
  else
    local_redefs.insert(obj);

  // Iterate over all uses of this object to find redefinitions.
  for (auto &use : this->value().uses()) {
    auto *user = use.getUser();

    Object user_obj(*user, obj.offsets());

    // Recurse on redefinitions.
    if (auto *phi = dyn_cast<llvm::PHINode>(user)) {
      gather_redefinitions(user_obj);

    } else if (auto *memoir_inst = into<MemOIRInst>(user)) {
      if (isa<RetPHIInst, UsePHIInst>(memoir_inst)) {
        gather_redefinitions(user_obj);

      } else if (auto *update = dyn_cast<UpdateInst>(memoir_inst)) {
        if (&use == &update->getObjectAsUse())
          gather_redefinitions(user_obj);

      } else if (auto *fold = into<FoldInst>(user)) {
        // Gather variable if folded on, or recurse on closed argument.

        if (&use == &fold->getInitialAsUse()) {
          // Gather uses of the accumulator argument.
          auto &accum = fold->getAccumulatorArgument();
          gather_redefinitions(Object(accum, obj.offsets()));

          // Gather uses of the resultant.
          gather_redefinitions(user_obj);

        } else if (&use == &fold->getObjectAsUse()) {

          // If the element argument is an object, gather uses of it.
          auto *elem_arg = fold->getElementArgument();
          if (elem_arg and Type::value_is_object(*elem_arg)) {

            // Try to match the access indices against the offsets.
            auto maybe_distance = fold->match_offsets(obj.offsets());
            if (not maybe_distance)
              continue;
            auto distance = maybe_distance.value();

            // If the offsets are not fully exhausted, recurse on the value
            // argument.
            if (distance < obj.offsets().size()) {

              // Construct the nested offsets.
              Vector<unsigned> nested_offsets(obj.offsets().begin(),
                                              obj.offsets().end());

              // Append the new offsets from the base.
              auto new_offset = obj.offsets().take_front(distance + 1);
              nested_offsets.insert(nested_offsets.end(),
                                    new_offset.begin(),
                                    new_offset.end());

              // Recurse.
              gather_redefinitions(Object(*elem_arg, nested_offsets));
            }
          }

        } else if (auto *closed_arg = fold->getClosedArgument(use)) {
          // Gather uses of the closed argument.
          gather_redefinitions(Object(*closed_arg, obj.offsets()));
        }
      }
    }
  }
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

void ObjectInfo::gather_uses_to_proxy(const Object &obj) {

  auto &value = obj.value();
  auto offsets = obj.offsets();
  auto &function =
      MEMOIR_SANITIZE(parent_function(value),
                      "Gathering uses of value with no parent function!");

  infoln("REDEF ", value, " IN ", function.getName());

  // From a given collection, V, gather all uses that need to be either
  // encoded or decoded.
  for (auto &use : value.uses()) {
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
              this->addkey(function, *index_use);
              continue;
            }
          }

          infoln("    ENCODING KEY ", *index_use->get());
          this->encode(function, *index_use);
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
            this->decode(fold->getBody(), index_arg);
            continue;
          }
        }

      } else if (auto input_kw = access->get_keyword<InputKeyword>()) {
        if (&input_kw->getInputAsUse() == &use) {
          auto &type =
              MEMOIR_SANITIZE(type_of(value),
                              "Failed to get type of object used as input.");
          if (auto *index_use = get_use_at_offset(input_kw->index_ops_begin(),
                                                  input_kw->index_ops_end(),
                                                  offsets)) {
            infoln("    ENCODING KEY ", *index_use->get());
            this->encode(function, *index_use);
            continue;
          }
        }
      }
    }
  }

  return;
}

void ObjectInfo::gather_uses_to_propagate(const Object &obj) {
  // Unpack the object, and fetch the local function.
  auto &value = obj.value();
  auto offsets = obj.offsets();
  auto &function = *this->function();

  infoln("REDEF ", value, " IN ", function.getName());

  // From a given collection, V, gather all uses that need to be either
  // encoded or decoded.
  for (auto &use : value.uses()) {
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
        if ((distance + 1) == offsets.size())
          if (auto *elem_arg = fold->getElementArgument())
            this->decode(fold->getBody(), *elem_arg);

      } else if (auto *read = dyn_cast<ReadInst>(access)) {
        if (distance == offsets.size())
          this->decode(function, read->asValue());

      } else if (auto *update = dyn_cast<UpdateInst>(access)) {
        // Handle values.
        if (auto *write = dyn_cast<WriteInst>(access)) {
          if (distance == offsets.size())
            this->addkey(function, write->getValueWrittenAsUse());

        } else if (auto *insert = dyn_cast<InsertInst>(access)) {
          if (auto value_kw = insert->get_keyword<ValueKeyword>())
            if (distance == offsets.size())
              this->addkey(function, value_kw->getValueAsUse());
        }
      }
    }
  }
}
void ObjectInfo::analyze() {
  println();
  println("ANALYZING ", *this);

  auto &base = this->value();

  gather_redefinitions();

  bool is_propagator = this->is_propagator();

  infoln("PROPAGATOR? ", is_propagator ? "YES" : "NO");

  auto offsets = this->offsets();

  for (const auto &[func, info] : this->info()) {
    for (const auto &redef : info.redefinitions) {
      auto redef_offsets = offsets.drop_front(redef.offsets().size());
      if (is_propagator)
        gather_uses_to_propagate(Object(redef.value(), redef_offsets));
      else
        gather_uses_to_proxy(Object(redef.value(), redef_offsets));
    }
  }
}

// Update
static llvm::Value *remap(llvm::Value *val, llvm::ValueToValueMapTy &vmap) {
  auto found = vmap.find(val);
  if (found == vmap.end()) {
    return val;
  }
  return found->second;
}

static llvm::Use *remap(llvm::Use *use, llvm::ValueToValueMapTy &vmap) {
  auto *user = use->getUser();
  auto *new_user = cast<llvm::User>(remap(user, vmap));
  return &new_user->getOperandUse(use->getOperandNo());
}

static void update_values(llvm::ValueToValueMapTy &vmap,
                          const Set<llvm::Value *> &input,
                          Set<llvm::Value *> &output) {
  for (auto *val : input)
    output.insert(remap(val, vmap));
}

static void update_values(llvm::ValueToValueMapTy &vmap,
                          const Set<Object> &input,
                          Set<Object> &output) {
  for (const auto &info : input)
    output.emplace(*remap(&info.value(), vmap), info.offsets());
}

static void update_uses(llvm::ValueToValueMapTy &vmap,
                        const Set<llvm::Use *> &input,
                        Set<llvm::Use *> &output) {
  for (auto *use : input)
    output.insert(remap(use, vmap));
}

#if 0
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

    if (func == &old_func) {

      new_bases[remap(val, vmap)] = remap(base, vmap);

      if (delete_old) {
        it = bases.erase(it);
        continue;
      }
    }

    ++it;
  }

  for (const auto &[val, base] : new_bases) {
    bases[val] = base;
  }
}
#endif

void ObjectInfo::update(llvm::Function &old_func,
                        llvm::Function &new_func,
                        llvm::ValueToValueMapTy &vmap,
                        bool delete_old) {

  // Update the base value.
  if (&old_func == this->function())
    this->value(*vmap[&this->value()]);

  // If we don't have any local info for the old function, skip.
  if (this->info().contains(&old_func))
    return;

  // Update the set of redefinitions.
  const auto &old_local = this->local(old_func);
  auto &new_local = this->local(new_func);

  // Update Redefinitions.
  update_values(vmap, old_local.redefinitions, new_local.redefinitions);

  // Update the uses.
  update_uses(vmap, old_local.to_encode, new_local.to_encode);
  update_uses(vmap, old_local.to_addkey, new_local.to_addkey);

  // Update the encoded values.
  update_values(vmap, old_local.encoded, new_local.encoded);

  // TODO: Update bases.

  // Delete the old function info, if need be.
  if (delete_old)
    this->info().erase(&old_func);

  return;
}

// BaseObjectInfo
AllocInst &BaseObjectInfo::allocation() const {
  return MEMOIR_SANITIZE(into<AllocInst>(this->value()),
                         "BaseObjectInfo with non-alloc value");
}

// ArgObjectInfo
llvm::Argument &ArgObjectInfo::argument() const {
  return MEMOIR_SANITIZE(dyn_cast<llvm::Argument>(&this->value()),
                         "ArgObjectInfo with non-arg value");
}

Map<llvm::CallBase *, ObjectInfo *> &ArgObjectInfo::incoming() {
  return this->_incoming;
}

const Map<llvm::CallBase *, ObjectInfo *> &ArgObjectInfo::incoming() const {
  return this->_incoming;
}

ObjectInfo *ArgObjectInfo::incoming(llvm::CallBase &call) const {
  auto found = this->incoming().find(&call);
  if (found == this->incoming().end())
    return NULL;
  return found->second;
}

void ArgObjectInfo::incoming(llvm::CallBase &call, ObjectInfo &obj) {
  this->incoming()[&call] = &obj;
}

} // namespace folio
