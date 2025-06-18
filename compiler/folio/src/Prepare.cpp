#include "memoir/ir/Builder.hpp"
#include "memoir/ir/CallGraph.hpp"
#include "memoir/support/SortedVector.hpp"

#include "folio/Mapping.hpp"
#include "folio/ProxyInsertion.hpp"
#include "folio/Utilities.hpp"
#include "folio/Version.hpp"

using namespace llvm::memoir;

namespace folio {

static SortedVector<llvm::CallBase *> enumerated_callers(
    llvm::Function &callee,
    const Vector<Candidate> &candidates) {

  SortedVector<llvm::CallBase *> callers = {};

  for (const auto &candidate : candidates) {
    for (const auto *info : candidate) {
      for (const auto &[func, pair] : info->redefinitions) {
        if (auto *caller = pair.first) {
          if (caller->getCalledFunction() == &callee) {
            callers.insert(caller);
          }
        }
      }
    }
  }

  return callers;
}

static void collect_versions(
    Set<llvm::Argument *> &arguments,
    Map<llvm::Function *, List<SortedVector<llvm::CallBase *>>> &versions,
    const Vector<Candidate> &candidates,
    const Mapping &mapping) {

  // For each base in the mapping.
  for (const auto &[base, global] : mapping.globals()) {
    if (auto *arg = dyn_cast<llvm::Argument>(base)) {

      // Track the argument.
      arguments.insert(arg);

      // Fetch the parent function.
      auto &func =
          MEMOIR_SANITIZE(arg->getParent(), "Argument has no parent function!");

      // If we've already seen this function, continue;
      if (versions.contains(&func)) {
        continue;
      }
      auto &func_versions = versions[&func];

      // Collect the set of enumerated callers.
      auto enum_callers = enumerated_callers(func, candidates);

      // Collect the set of external callers to this function.
      auto callers = possible_callers(func);
      func_versions.emplace_back();
      auto &external_callers = func_versions.back();
      for (auto *caller : callers) {
        external_callers.insert(caller);
      }
      external_callers.set_difference(enum_callers);

      // TODO: partition the enumerated callers into more versions based on
      // arguments aligning.
      if (not enum_callers.empty()) {
        func_versions.push_back(enum_callers);
      }
    }
  }
}

static void cleanup_versions(
    Map<llvm::Function *, List<SortedVector<llvm::CallBase *>>> &versions) {
  // Eliminate empty versions.
  for (auto it = versions.begin(); it != versions.end();) {

    auto &func_versions = it->second;

    for (auto jt = std::next(func_versions.begin());
         jt != func_versions.end();) {
      if (jt->empty()) {
        jt = func_versions.erase(jt);
      } else {
        ++jt;
      }
    }

    // If we only have the original version, no cloning is needed.
    if (func_versions.size() <= 1) {
      it = versions.erase(it);
    } else {
      ++it;
    }
  }
}

static void monomorphize(Vector<Candidate> &candidates) {

  // Find all abstract arguments.
  Map<llvm::Argument *, Set<llvm::CallBase *>> abstract_args = {};
  for (auto &candidate : candidates) {
    for (auto *info : candidate) {
      for (const auto &[func, context] : info->redefinitions) {
        // Unpack the context.
        const auto &[call, base_redefs] = context;

        // Skip context-insensitive results.
        if (not call) {
          continue;
        }

        // Map relevant arguments to their possible callees.
        for (const auto &[base, redefs] : base_redefs) {
          if (auto *arg = dyn_cast<llvm::Argument>(base)) {
            abstract_args[arg].insert(call);
          }
        }
      }
    }
  }

  // Partition the calls based on pattern.
  Set<llvm::Function *> callees = {};
  Map<llvm::CallBase *, Set<llvm::Argument *>> call_args = {};
  for (const auto &[arg, calls] : abstract_args) {

    // Debug print.
    println("ARG ", *arg, " IN ", arg->getParent()->getName());
    for (auto *call : calls) {
      println("  CALL ", *call);
    }

    // Track the callee.
    callees.insert(arg->getParent());

    // Construct a pivot table.
    for (auto &call : calls) {
      call_args[call].insert(arg);
    }
  }

  // Determine which function versions we need.
  Map<llvm::Function *, List<SortedVector<llvm::CallBase *>>> versions = {};
  for (auto *func : callees) {
    auto &func_versions = versions[func];

    // Determine if there are callers to the original function.
    auto callers = possible_callers(*func);
    func_versions.emplace_back();
    auto &orig_version = func_versions.back();
    for (auto *caller : callers) {
      orig_version.insert(caller);
    }

    // Create versions for each enumerated call.
    Set<llvm::CallBase *> used = {};
    while (call_args.size() > used.size()) {
      for (auto it = call_args.begin(); it != call_args.end(); ++it) {
        const auto &[call, args] = *it;

        if (used.contains(call)) {
          continue;
        }

        // Create a new version.
        func_versions.emplace_back();
        auto &curr_version = func_versions.back();
        curr_version.insert(call);

        // Find other calls that match this ones args.
        for (auto jt = std::next(it); jt != call_args.end(); ++jt) {
          const auto &[other_call, other_args] = *jt;

          if (used.contains(other_call)) {
            continue;
          }

          // Check if the args match.
          if (args.size() != other_args.size()) {
            continue;
          }
          bool all_found = true;
          for (auto *arg : args) {
            if (not other_args.contains(arg)) {
              all_found = false;
              break;
            }
          }
          if (not all_found) {
            continue;
          }

          // If the args match, add this call to the version.
          curr_version.insert(other_call);
        }

        // Mark all calls in the current version as used.
        used.insert(curr_version.begin(), curr_version.end());

        // Remove calls in this version from the set of otiginal calls.
        orig_version.set_difference(curr_version);
      }
    }

    if (not orig_version.empty()) {
      // Clone the function and update the argument mapping.
      MEMOIR_UNREACHABLE("Need original, versioning is unimplemented!");
    }

    if (func_versions.size() > 2) {
      MEMOIR_UNREACHABLE("Need version, versioning is unimplemented!");
    }
  }

  for (const auto &[func, func_versions] : versions) {
    println("VERSIONS OF ", func->getName());
    for (const auto &version : func_versions) {
      if (version.empty()) {
        println(" NO ORIGINAL NEEDED");
        continue;
      }

      println(" VERSION");
      for (auto *call : version) {
        println("  ", *call);
      }
    }
  }
}

static llvm::Value *find_base_of(llvm::Value &value,
                                 const Vector<Candidate *> &candidates) {

  auto *func = parent_function(value);

  llvm::Value *found_base = NULL;
  for (auto *candidate_ptr : candidates) {
    auto &candidate = *candidate_ptr;

    for (auto *info : candidate) {
      // Search the redefinitions for the value.
      auto [lo, hi] = info->redefinitions.equal_range(func);
      for (auto it = lo; it != hi; ++it) {
        for (const auto &[base, locals] : it->second.second) {

          bool found = false;
          for (const auto &local : locals) {
            if (&local.value() == &value) {
              found = true;
              break;
            }
          }

          if (found) {
            if (not found_base) {
              found_base = base;
            } else if (found_base != base) {
              MEMOIR_UNREACHABLE("Found base ",
                                 *found_base,
                                 " does not match base ",
                                 *base);
            }
          }
        }
      }
    }
  }

  return found_base;
}

static void create_base_globals(Vector<Candidate> &candidates) {

  // Find all of the bases in the program.
  Map<llvm::Value *, SmallMap<Candidate *, SmallSet<ObjectInfo *>>>
      base_objects = {};
  for (auto &candidate : candidates) {
    // Collect all of the bases and their relevant objects in this candidate.
    for (auto *info : candidate) {
      for (const auto &[func, context] : info->redefinitions) {
        for (const auto &[base, _redefs] : context.second) {
          base_objects[base][&candidate].insert(info);
        }
      }
    }
  }

  // Create a global for each of the bases, and update the mapping.
  Map<llvm::Value *, Pair<llvm::GlobalVariable *, llvm::GlobalVariable *>>
      mappings = {};
  Map<llvm::Value *, Vector<Candidate *>> base_candidates = {};
  for (const auto &[base, candidate_objects] : base_objects) {

    auto &function =
        MEMOIR_SANITIZE(parent_function(*base), "Base has no parent function");
    auto &module =
        MEMOIR_SANITIZE(function.getParent(), "Function has no parent module");

    // Create globals for this base.
    auto &enc = create_global_ptr(module, "enc.");
    auto &dec = create_global_ptr(module, "dec.");

    // TODO: we need to differentiate different NestedObjects within the base.
    mappings[base].first = &enc;
    mappings[base].second = &dec;

    // Update the mapping in each candidate.
    for (const auto &[candidate, objects] : candidate_objects) {

      base_candidates[base].push_back(candidate);

      candidate->encoder.global(base, enc);
      candidate->decoder.global(base, dec);
    }
  }

  // Link the caller-callee for each base.
  for (const auto &[base, globals] : mappings) {
    const auto &[enc, dec] = globals;

    auto *arg = dyn_cast<llvm::Argument>(base);
    if (not arg) {
      // Non-argument bases will be handled later.
      continue;
    }

    // Collect the callers for this function.
    auto &func = MEMOIR_SANITIZE(arg->getParent(), "Arg has no parent");

    auto callers = possible_callers(func);

    for (auto *call : callers) {
      auto *operand = call->getOperand(arg->getArgNo());
      auto *caller_base = find_base_of(*operand, base_candidates[base]);

      if (not caller_base) {
        MEMOIR_UNREACHABLE("Failed to find base of ", *operand, " for ", *call);
      }

      // Get the globals for the caller base.
      const auto &[caller_enc, caller_dec] = mappings.at(caller_base);

      // Load the mappings and store them to the globals.
      MemOIRBuilder builder(call);

      auto *enc_type = enc->getValueType();
      auto *enc_value = builder.CreateLoad(enc_type, caller_enc);
      builder.CreateStore(enc_value, enc);

      auto *dec_type = dec->getValueType();
      auto *dec_value = builder.CreateLoad(dec_type, caller_dec);
      builder.CreateStore(dec_value, dec);
    }
  }
}

void ProxyInsertion::prepare() {

  // Monomorphize the program, for candidate patterns.
  monomorphize(this->candidates);

  // For each base in the candidate, insert a global for it.
  create_base_globals(this->candidates);
}

} // namespace folio
