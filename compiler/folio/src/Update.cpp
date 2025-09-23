#include "folio/Candidate.hpp"

using namespace memoir;

namespace folio {

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

  // Delete the old function info, if need be.
  if (delete_old)
    this->info().erase(&old_func);

  return;
}

// Update analysis results following a function clone.
void Candidate::update(llvm::Function &old_func,
                       llvm::Function &new_func,
                       llvm::ValueToValueMapTy &vmap,
                       bool delete_old) {
  // Update each of the objects in this candidate.
  for (auto *info : *this)
    info->update(old_func, new_func, vmap, delete_old);
}

} // namespace folio
