#include "llvm/Support/CommandLine.h"

#include "memoir/ir/CallGraph.hpp"
#include "memoir/support/SortedVector.hpp"
#include "memoir/support/WorkList.hpp"

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

void ProxyInsertion::flesh_out(Candidate &candidate) {
  // Discover all relevant abstract objects for this candidate, and insert them.

  // Gather the set of outgoing objects.
  Map<ObjectInfo *, Vector<ArgObjectInfo *>> outgoing;
  for (auto &arg : this->arguments)
    for (const auto &[call, incoming] : arg.incoming())
      outgoing[incoming].push_back(&arg);

  // Keep adding objects until a fixed point is reached.
  WorkList<ObjectInfo *, /* VisitOnce? */ true> worklist;
  worklist.push(candidate.begin(), candidate.end());
  while (not worklist.empty()) {
    auto *obj = worklist.pop();

    auto found = outgoing.find(obj);
    if (found == outgoing.end())
      continue;

    for (auto *out : found->second)
      if (worklist.push(out))
        candidate.push_back(out);
  }
}
void ProxyInsertion::gather_assoc_objects(AllocInst &alloc) {
  this->gather_assoc_objects(alloc, alloc.getType());
}

void ProxyInsertion::gather_assoc_objects(AllocInst &alloc,
                                          Type &type,
                                          Offsets offsets) {
  if (auto *tuple_type = dyn_cast<TupleType>(&type)) {
    for (unsigned field = 0; field < tuple_type->getNumFields(); ++field) {

      auto new_offsets = offsets;
      new_offsets.push_back(field);

      this->gather_assoc_objects(alloc,
                                 tuple_type->getFieldType(field),
                                 new_offsets);
    }

  } else if (auto *collection_type = dyn_cast<CollectionType>(&type)) {
    auto &elem_type = collection_type->getElementType();

    // If this is an assoc, add the object information.
    if (isa<AssocType>(collection_type)) {
      this->objects.emplace_back(alloc, offsets);
    }

    // Recurse on the element.
    auto new_offsets = offsets;
    new_offsets.push_back(-1);

    this->gather_assoc_objects(alloc, elem_type, new_offsets);
  }

  return;
}

static bool object_can_share(Type &type,
                             llvm::Function *func,
                             ObjectInfo &other) {
  // Check that the key types match.
  auto &other_type = MEMOIR_SANITIZE(dyn_cast<AssocType>(&other.type()),
                                     "Non-assoc type, unhandled.");
  if (&type != &other_type.getKeyType())
    return false;

  // Check that they share a parent function.
  // NOTE: this is overly conservative
  if (func != other.function())
    return false;

  return true;
}

static bool propagator_can_share(Type &type,
                                 llvm::Function *func,
                                 ObjectInfo &other) {
  if (&type != &other.type()) {
    return false;
  }

  // Check that they share a parent basic block.
  if (func != other.function()) {
    return false;
  }

  return true;
}

void ProxyInsertion::gather_propagators(const Set<Type *> &types,
                                        AllocInst &alloc) {
  this->gather_propagators(types, alloc, alloc.getType());
}

void ProxyInsertion::gather_propagators(const Set<Type *> &types,
                                        AllocInst &alloc,
                                        Type &type,
                                        OffsetsRef offsets) {

  if (auto *collection_type = dyn_cast<CollectionType>(&type)) {

    auto &elem_type = collection_type->getElementType();

    Offsets nested_offsets(offsets.begin(), offsets.end());

    // Recurse on the element.
    nested_offsets.push_back(unsigned(-1));
    this->gather_propagators(types, alloc, elem_type, nested_offsets);

  } else if (auto *tuple_type = dyn_cast<TupleType>(&type)) {

    Offsets nested_offsets(offsets.begin(), offsets.end());

    for (Offset field = 0; field < tuple_type->getNumFields(); ++field) {
      auto &field_type = tuple_type->getFieldType(field);

      // Recurse on the field.
      nested_offsets.push_back(field);
      this->gather_propagators(types, alloc, field_type, nested_offsets);
      nested_offsets.pop_back();
    }

  } else if (types.contains(&type)) {
    this->propagators.emplace_back(alloc, offsets);
  }
}

#if 0
static void add_enumerated(ProxyInsertion::Enumerated &enumerated,
                           WorkList<NestedObject> &worklist,
                           llvm::Value &value,
                           llvm::ArrayRef<Offset> offsets,
                           const SmallSet<Candidate *, 1> &incoming) {
  NestedObject obj(value, offsets);

  auto [it, fresh] = enumerated.try_emplace(obj, incoming);
  if (not fresh) {
    auto &enums = it->second;
    auto orig_size = enums.size();
    enums.insert(incoming.begin(), incoming.end());
    if (enums.size() == orig_size)
      return;
  }

  worklist.push(obj);
}

static void gather_enumerated(Vector<Candidate> &candidates,
                              ProxyInsertion::Enumerated &enumerated) {
  // For each candidate, map the allocations to their candidate.
  for (auto &candidate : candidates) {
    for (auto *info : candidate) {
      auto &alloc = info->allocation->asValue();
      NestedObject obj(alloc, info->offsets);
      enumerated[obj].insert(&candidate);
    }
  }

  // Propagate the candidate information along the def-use chains.
  WorkList<NestedObject> worklist;
  for (const auto &[obj, _] : enumerated)
    worklist.push(obj);

  while (not worklist.empty()) {
    auto obj = worklist.pop();
    auto &enums = enumerated[obj];

    // Propagate to each user of the object.
    for (auto &use : obj.value().uses()) {
      auto *user = dyn_cast<llvm::Instruction>(use.getUser());
      if (not user)
        continue;

      if (auto *phi = dyn_cast<llvm::PHINode>(user)) {
        add_enumerated(enumerated, worklist, *phi, obj.offsets(), enums);

      } else if (auto *memoir = into<MemOIRInst>(user)) {
        if (auto *update = dyn_cast<UpdateInst>(memoir)) {
          if (&use == &update->getObjectAsUse())
            add_enumerated(enumerated,
                           worklist,
                           update->getResult(),
                           obj.offsets(),
                           enums);

        } else if (auto *retphi = dyn_cast<RetPHIInst>(memoir)) {
          add_enumerated(enumerated,
                         worklist,
                         retphi->getResult(),
                         obj.offsets(),
                         enums);

        } else if (auto *fold = dyn_cast<FoldInst>(memoir)) {
          if (&use == &fold->getObjectAsUse()) {
            auto match = fold->match_offsets(obj.offsets());
            if (match and (1 + match.value()) <= obj.offsets().size())
              if (auto *arg = fold->getElementArgument())
                add_enumerated(enumerated,
                               worklist,
                               *arg,
                               obj.offsets().drop_front(match.value() + 1),
                               enums);

          } else if (&use == &fold->getInitialAsUse()) {
            add_enumerated(enumerated,
                           worklist,
                           fold->getAccumulatorArgument(),
                           obj.offsets(),
                           enums);
            add_enumerated(enumerated,
                           worklist,
                           fold->getResult(),
                           obj.offsets(),
                           enums);

          } else if (auto *arg = fold->getClosedArgument(use)) {
            add_enumerated(enumerated, worklist, *arg, obj.offsets(), enums);
          }
        }
      } else if (auto *call = dyn_cast<llvm::CallBase>(user)) {
        // Create an abstract candidate with alias information.
        auto &function = MEMOIR_SANITIZE(call->getCalledFunction(),
                                         "Found indirect function call");
        auto &arg = MEMOIR_SANITIZE(function.getArg(use.getOperandNo()),
                                    "Failed to get argument");

        // If we have already handled this argument, skip it.
        NestedObject arg_obj(arg, obj.offsets());

        // Collect all of the possible callers.
        auto callers = possible_callers(function);

        // If this function has a single caller with only one incoming
        // candidate, then simply propagate.
        if (single_caller(function) == call and enums.size() == 1) {
          add_enumerated(enumerated, worklist, arg, obj.offsets(), enums);
        }

        // Otherwise, create or update the abstract candidate.
        auto *abstract = new AbstractCandidate();
        auto [it, fresh] = enumerated.try_emplace(arg_obj);
        if (not fresh) {
          delete abstract;
          abstract = cast<AbstractCandidate>(*it->second.begin());
        } else {
          it->second.insert(abstract);
        }

        // Add all of the aliasing enums.
        auto orig_size = abstract->aliased.size();
        abstract->aliased.insert(enums.begin(), enums.end());

        if (orig_size < abstract->aliased.size())
          worklist.push(arg_obj);
      }
    }
  }

  // DEBUG PRINT
  println("ENUMERATED =================================");
  for (const auto &[obj, enums] : enumerated) {
    println(" OBJ ", obj);
    println("   # ", enums.size());
  }
  println("============================================");
}
#endif
static llvm::Function &get_alloc_function(llvm::Module &module) {
  return MEMOIR_SANITIZE(
      FunctionNames::get_memoir_function(module, MemOIR_Func::ALLOCATE),
      "Failed to find MEMOIR alloc function!");
}

void ProxyInsertion::gather_assoc_objects() {
  for (auto &use : get_alloc_function(this->module).uses())
    if (auto *alloc = into<AllocInst>(use.getUser()))
      this->gather_assoc_objects(*alloc);

  for (auto &info : this->objects)
    info.analyze();
}

void ProxyInsertion::gather_propagators() {
  // Collect the key types of our assoc objects.
  Set<Type *> types = {};
  for (auto &info : this->objects)
    if (auto *assoc_type = dyn_cast<AssocType>(&info.type()))
      types.insert(&assoc_type->getKeyType());

  // Gather all objects in the program with elements of a relevant type.
  for (auto &use : get_alloc_function(this->module).uses())
    if (auto *alloc = into<AllocInst>(use.getUser()))
      this->gather_propagators(types, *alloc);

  // Analyze each of the propagators.
  for (auto &info : this->propagators)
    info.analyze();
}

void ProxyInsertion::gather_abstract_objects(ObjectInfo &obj) {
  for (const auto &[func, local] : obj.info()) {
    for (const auto &redef : local.redefinitions) {
      for (auto &use : redef.value().uses()) {
        auto *call = dyn_cast<llvm::CallBase>(use.getUser());
        // Skip non-calls and memoir instructions.
        if (not call or into<MemOIRInst>(call))
          continue;

        auto *function = call->getCalledFunction();
        MEMOIR_ASSERT(function, "NYI, blacklist object used by indirect call");
        auto *arg = function->getArg(use.getOperandNo());
        Object arg_obj(*arg, obj.offsets());

        // If the arg object doesn't exist yet, create one.
        auto begin = this->arguments.begin(), end = this->arguments.end();
        auto found = std::find_if(begin, end, [&arg_obj](const Object &other) {
          return arg_obj == other;
        });

        bool fresh = false;
        ArgObjectInfo *arg_info = NULL;
        if (found == end) {
          fresh = true;
          arg_info = &this->arguments.emplace_back(*arg, obj.offsets());
        } else {
          arg_info = &*found;
        }

        // Add the incoming call edge.
        arg_info->incoming(*call, obj);

        // If the object was newly added, analyze it and recurse.
        if (fresh) {
          arg_info->analyze();
          this->gather_abstract_objects(*arg_info);
        }
      }
    }
  }
}

void ProxyInsertion::gather_abstract_objects() {
  // Find any call users of the base object, and construct object infos for the
  // argument.
  for (auto &obj : this->objects)
    this->gather_abstract_objects(obj);
  for (auto &obj : this->propagators)
    this->gather_abstract_objects(obj);
  for (auto &arg : this->arguments)
    this->gather_abstract_objects(arg);
}

void ProxyInsertion::share_proxies() {

  Set<const ObjectInfo *> used = {};
  for (auto it = this->objects.begin(); it != this->objects.end(); ++it) {
    auto &info = *it;

    if (used.count(&info)) {
      continue;
    }

    auto *func = info.function();

    auto &type = MEMOIR_SANITIZE(dyn_cast<AssocType>(&info.type()),
                                 "Non-assoc type, unhandled.");
    auto &key_type = type.getKeyType();

    this->candidates.emplace_back(key_type);
    auto &candidate = this->candidates.back();
    candidate.push_back(&info);
    this->flesh_out(candidate);

    // Compute the benefit of the candidate as is.
    candidate.benefit = benefit(candidate).benefit;

    // Find all other allocations in the same function as this one.
    if (not disable_proxy_sharing) {

      // Collect the set of objects that can share, and their solo benefit.
      Map<ObjectInfo *, int> shareable = {};

      for (auto &other : objects) {
        if (used.contains(&other) or &other == &info)
          continue;

        if (object_can_share(key_type, func, other)) {
          Candidate other_alone(key_type, { &other });
          this->flesh_out(other_alone);
          shareable[&other] = benefit(other_alone).benefit;
        }
      }

      // Find all propagators in the same function as this one.
      for (auto &other : propagators) {
        if (used.contains(&other) or &other == &info)
          continue;

        if (propagator_can_share(key_type, func, other)) {
          Candidate other_alone(key_type, { &other });
          this->flesh_out(other_alone);
          shareable[&other] = benefit(other_alone).benefit;
        }
      }

      println("SHAREABLE");
      for (const auto &[other, _] : shareable)
        println("  ", *other);
      println();

      // Iterate until we can't find a new object to add to the candidate.
      bool fresh;
      do {
        fresh = false;

        println("CURRENT ", candidate);
        println("  BENEFIT ", candidate.benefit);

        for (auto jt = shareable.begin(); jt != shareable.end();) {
          const auto &[other, single_benefit] = *jt;

          println("  WHAT IF? ", *other);

          // Compute the benefit of adding this candidate.
          Candidate checkpoint = candidate;
          candidate.push_back(other);
          this->flesh_out(candidate);
          auto new_heuristic = benefit(candidate);
          auto new_benefit = new_heuristic.benefit;
          println("    NEW BENEFIT ", new_heuristic.benefit);
          println("    SUM BENEFIT ", candidate.benefit + single_benefit);
          println("     ADDED COST ", new_heuristic.cost);
          println();

          if (new_benefit > (candidate.benefit + single_benefit)
              or new_heuristic.cost == 0) {
            fresh = true;
            candidate.benefit = new_benefit;

            // Erase the object from the search list.
            jt = shareable.erase(jt);

          } else {
            // If there's no benefit, roll it back.
            candidate = std::move(checkpoint);

            // Continue iterating.
            ++jt;
          }
        }

      } while (fresh);
    }

    // If there is no benefit, roll back the candidate.
    if (candidate.size() == 0 or candidate.benefit == 0)
      this->candidates.pop_back();
    else
      used.insert(candidate.begin(), candidate.end());
  }

  // Give each candidate a unique ID.
  int id = 0;
  for (auto &candidate : this->candidates)
    candidate.id = ++id;

  println("=== CANDIDATES ===");
  for (auto &candidate : this->candidates) {
    println("CANDIDATE ", candidate.id);
    println("  IN ", candidate.function().getName());
    println("  BENEFIT=", candidate.benefit);
    for (const auto *info : candidate) {
      println("  ", *info);
    }
    println();
  }
  println();
}

void ProxyInsertion::unify_bases() {

  println("=== UNIFY BASES ===");
  this->unified.clear();
  this->equiv.clear();

  WorkList<ArgObjectInfo *, /* VisitOnce? */ true> worklist;

  { // Unify bases within each candidate.
    for (auto &candidate : this->candidates) {
      ObjectInfo *first = NULL;
      for (auto *obj : candidate) {
        // Insert the object.
        this->unified.insert(obj);

        // Unify all base objects within this candidate.
        if (auto *base = dyn_cast<BaseObjectInfo>(obj)) {
          if (not first)
            first = base;
          else
            unified.merge(first, base);
        } else if (auto *arg = dyn_cast<ArgObjectInfo>(obj)) {
          worklist.push(arg);
        }
      }
    }

    // Debug print.
    debugln(" >> INITIALIZED BASE OBJECTS << ");
    for (const auto &[obj, _] : this->unified)
      debugln("    ", *obj);
    debugln();
  }

  { // Unify arguments across candidate.

    // Construct the outgoing mapping from obj->arg and group args by their
    // parent function.
    Map<ObjectInfo *, Set<ArgObjectInfo *>> outgoing;
    Map<llvm::Function *, Vector<ArgObjectInfo *>> func_args;
    for (auto *arg : worklist) {
      func_args[arg->function()].push_back(arg);
      for (const auto &[call, incoming] : arg->incoming())
        outgoing[incoming].insert(arg);
    }

    // Unify arguments until a fixed point is reached.
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
          auto *other_inc = other->incoming(*call);

          auto inc_base = this->unified.try_find(inc);
          auto other_inc_base = this->unified.try_find(other_inc);
          if (not inc_base or not other_inc_base
              or inc_base.value() != other_inc_base.value()) {
            all_same = false;
            break;
          }
        }

        if (all_same) {
          this->unified.merge(arg, other);
          worklist.push(other);
        }
      }

      // Merge arguments if all incoming objects are merged.
      ObjectInfo *shared = NULL;
      for (const auto &[call, incoming] : arg->incoming()) {
        if (not this->unified.contains(incoming))
          continue;

        auto *inc_base = this->unified.find(incoming);
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
      if (this->unified.find(arg) == shared)
        continue;

      // If we found a single merge argument, merge with this argument.
      debugln(" --> MERGE");
      debugln("       ", *arg);
      debugln("       ", *shared);
      this->unified.merge(arg, shared);

      // Enqueue all outgoing.
      if (outgoing.contains(arg))
        for (auto *out : outgoing.at(arg))
          worklist.push(out);
    }

    // Debug print.
    debugln(" >> AFTER FIXED POINT << ");
    for (const auto &[obj, parent] : this->unified)
      debugln("    ", *obj, " -> ", *parent);
    debugln();
  }

  { // Reify the unified objects.
    this->unified.reify();

    debugln(" >> AFTER REIFY << ");
    for (const auto &[base, _] : this->unified)
      debugln("    ", *base);
    debugln();
  }

  { // Construct the set of equivalence classes.
    for (const auto &[obj, parent] : this->unified)
      this->equiv[parent].push_back(obj);

    println("=== EQUIVALENCE CLASSES ===");
    for (const auto &[base, objects] : this->equiv) {
      println(" ┌╼ ", *base);
      for (const auto &obj : objects)
        println(" ├──╼ ", *obj);
      println(" └╼ ");
    }
  }
}

void ProxyInsertion::analyze() {
  // Gather all assoc object allocations in the program.
  this->gather_assoc_objects();

  println();
  println("FOUND OBJECTS ", this->objects.size());
  for (auto &info : this->objects) {
    println("  ", info);
  }
  println();

  // With the set of values that need to be encoded/decoded, we will find
  // collections that can be used to propagate proxied values.
  if (not disable_proxy_propagation) {

    // Gather all objects in the program that have elements of a relevant type.
    this->gather_propagators();

    println();
    println("FOUND PROPAGATORS ", this->propagators.size());
    for (auto &info : this->propagators) {
      println("  ", info);
    }
    println();
  }

  // Gather any abstract objects.
  this->gather_abstract_objects();

  // Use a heuristic to share proxies between collections.
  this->share_proxies();

  // Unify bases.
  this->unify_bases();
}

} // namespace folio
