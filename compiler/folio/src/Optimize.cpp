#include "memoir/ir/CallGraph.hpp"
#include "memoir/support/UnionFind.hpp"
#include "memoir/support/WorkList.hpp"

#include "folio/Candidate.hpp"
#include "folio/ProxyInsertion.hpp"
#include "folio/RedundantTranslations.hpp"
#include "folio/Utilities.hpp"
#include "folio/WeakenUses.hpp"

using namespace llvm::memoir;

namespace folio {

void Candidate::unify_bases() {
  auto &candidate = *this;

  println("UNIFY BASES");

  // Initialize the bases.
  this->bases.clear();
  for (auto *info : candidate)
    this->bases.insert(info);

  // Unify all base objects.
  ObjectInfo *first = NULL;
  WorkList<ArgObjectInfo *, /* VisitOnce? */ true> worklist;
  for (auto *info : candidate)
    if (auto *base = dyn_cast<BaseObjectInfo>(info)) {
      if (first)
        this->bases.merge(first, info);
      else
        first = info;
    } else if (auto *arg = dyn_cast<ArgObjectInfo>(info)) {
      worklist.push(arg);
    }

  // Outgoing call edges.
  Map<ObjectInfo *, Set<ArgObjectInfo *>> outgoing;
  Map<llvm::Function *, Vector<ArgObjectInfo *>> func_args;
  for (auto *arg : worklist) {
    func_args[arg->function()].push_back(arg);
    for (const auto &[call, incoming] : arg->incoming())
      outgoing[incoming].insert(arg);
  }

  // Iteratively unify arguments.
  while (not worklist.empty()) {
    auto *arg = worklist.pop();

    // If two arguments have the same incoming parents for all incoming edges,
    // merge them.
    auto *function = arg->function();
    for (auto *other : func_args.at(function)) {
      if (other == arg)
        continue;

      bool all_same = true;
      for (const auto &[call, inc] : arg->incoming()) {
        auto *inc_base = this->bases.find(inc);
        auto *other_inc_base = this->bases.find(other->incoming(*call));
        if (inc_base != other_inc_base) {
          all_same = false;
          break;
        }
      }

      if (all_same) {
        this->bases.merge(arg, other);
        worklist.push(other);
      }
    }

    // Merge arguments if all incoming objects are merged.
    ObjectInfo *shared = NULL;
    for (const auto &[call, incoming] : arg->incoming()) {
      auto *inc_base = this->bases.find(incoming);
      if (not shared)
        shared = inc_base;

      // If the bases differ, fail.
      if (inc_base != shared) {
        shared = NULL;
        break;
      }
    }

    // If we couldn't find a unified base, continue.
    if (not shared)
      continue;

    // If the arg base is already unified, continue.
    if (this->bases.find(arg) == shared)
      continue;

    // If we found a single merge argument, merge with this argument.
    this->bases.merge(arg, shared);

    // Enqueue all outgoing.
    if (outgoing.contains(arg))
      for (auto *out : outgoing.at(arg))
        worklist.push(out);
  }

  // Finally, reify the bases.
  this->bases.reify();

  // Construct the set of equivalence classes.
  this->equiv.clear();
  for (const auto &[v, p] : this->bases)
    this->equiv[p].push_back(v);

  // Debug print.
  println("=== BASES ===");
  for (const auto &[base, objects] : this->equiv) {
    println(" = ", *base);
    for (const auto &obj : objects)
      println(" ↳ ", *obj);
  }
}

void Candidate::gather_uses() {

  // Unify the bases and compute the equivalence classes.
  this->unify_bases();

  // Gather uses and encoded values within each equivalence class.
  for (const auto &[base, equiv] : this->equiv)
    for (auto *obj : equiv)
      for (const auto &[func, local] : obj->info()) {
        this->encoded[base][func].insert(local.encoded.begin(),
                                         local.encoded.end());
        this->to_encode[base][func].insert(local.to_encode.begin(),
                                           local.to_encode.end());
        this->to_decode[base][func].insert(local.to_addkey.begin(),
                                           local.to_addkey.end());
      }

  // Perform a forward analysis.
  for (auto &[base, values] : this->encoded)
    forward_analysis(values);

  // Collect the set of uses to decode.
  for (auto &[base, equiv_encoded] : this->encoded)
    for (const auto &[func, values] : equiv_encoded)
      for (auto *val : values)
        for (auto &use : val->uses()) {
          auto *user = use.getUser();

          // If the user is a PHI/Select/Fold _and_ is also encoded, we don't
          // need to decode it.
          if (isa<llvm::PHINode>(user) or isa<llvm::SelectInst>(user)) {
            if (values.contains(user))
              continue;

          } else if (auto *fold = into<FoldInst>(user)) {
            auto *arg = (&use == &fold->getInitialAsUse())
                            ? &fold->getAccumulatorArgument()
                            : fold->getClosedArgument(use);
            if (equiv_encoded[&fold->getBody()].count(arg))
              continue;

          } else if (auto *ret = dyn_cast<llvm::ReturnInst>(user)) {
            if (auto *func = ret->getFunction()) {
              if (auto *fold = FoldInst::get_single_fold(*func)) {
                auto *caller = fold->getFunction();
                auto &result = fold->getResult();
                // If the result of the fold is encoded, we don't decode here.
                if (equiv_encoded[caller].count(&result))
                  continue;
              }
            }
          } else if (auto *call = dyn_cast<llvm::CallBase>(user)) {
            // TODO
          }

          // Otherwise, mark the use for decoding.
          this->to_decode[base][func].insert(&use);
        }
}

void Candidate::optimize(
    std::function<llvm::DominatorTree &(llvm::Function &)> get_domtree,
    std::function<BoundsCheckResult &(llvm::Function &)> get_bounds_checks) {
  // Collect all of the uses that need to be handled.
  this->gather_uses();

  println("  FOUND USES");
  println("    TO ENCODE");
  print_uses(this->to_encode);
  println("    TO DECODE");
  print_uses(this->to_decode);
  println("    TO ADDKEY");
  print_uses(this->to_addkey);

#if 0
  if (not disable_use_weakening) {
    // DISABLED: Use weakening works, but does not have any considerable
    // performance benefits.

    // Weaken uses from addkey to encode if we know that the value is already
    // inserted.
    Set<llvm::Use *> to_weaken = {};
    weaken_uses(to_addkey, to_weaken, *this, get_bounds_checks);

    erase_uses(to_addkey, to_weaken);
    for (auto *use : to_weaken) {
      to_encode.insert(use);
    }
  }
#endif

  for (const auto &[base, encoded] : this->encoded)
    eliminate_redundant_translations(encoded,
                                     this->to_decode[base],
                                     this->to_encode[base],
                                     this->to_addkey[base]);

  println("  TRIMMED USES:");
  println("    TO ENCODE");
  print_uses(this->to_encode);
  println("    TO DECODE");
  print_uses(this->to_decode);
  println("    TO ADDKEY");
  print_uses(this->to_addkey);
}

void ProxyInsertion::optimize() {
  // Optimize the uses in each candidate.
  for (auto &candidate : this->candidates) {
    infoln("OPTIMIZING ", candidate);
    candidate.optimize(this->get_dominator_tree, this->get_bounds_checks);
  }
}

} // namespace folio
