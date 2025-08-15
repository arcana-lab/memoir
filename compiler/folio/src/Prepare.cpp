#include "memoir/ir/Builder.hpp"
#include "memoir/ir/CallGraph.hpp"
#include "memoir/support/SortedVector.hpp"

#include "folio/Mapping.hpp"
#include "folio/ProxyInsertion.hpp"
#include "folio/Utilities.hpp"
#include "folio/Version.hpp"

using namespace llvm::memoir;

namespace folio {

#if 0
static SortedVector<llvm::CallBase *> enumerated_callers(
    llvm::Function &callee,
    const Vector<Candidate> &candidates) {

  SortedVector<llvm::CallBase *> callers = {};

  for (const auto &candidate : candidates)
    for (const auto *info : candidate)
      if (auto *arg = dyn_cast<ArgObjectInfo>(info))
        for (const auto &[call, _] : arg->incoming())
          callers.insert(call);
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

#endif

static void store_mappings_for_base(llvm::Instruction *insertion_point,
                                    llvm::GlobalVariable *enc,
                                    llvm::GlobalVariable *caller_enc,
                                    llvm::GlobalVariable *dec,
                                    llvm::GlobalVariable *caller_dec) {

  // Load the mappings and store them to the globals.
  MemOIRBuilder builder(insertion_point);

  if (enc and caller_enc) {
    auto *enc_type = enc->getValueType();
    auto *enc_value = builder.CreateLoad(enc_type, caller_enc);
    builder.CreateStore(enc_value, enc);
  }

  if (dec and caller_dec) {
    auto *dec_type = dec->getValueType();
    auto *dec_value = builder.CreateLoad(dec_type, caller_dec);
    builder.CreateStore(dec_value, dec);
  }
}

static void create_base_globals(Vector<Candidate> &candidates) {

  // We need to create a global for each base object in the program, but the
  // same object may be shared between multiple candidates. So, we gather them
  // all here and record the candidates that we'll have to update later.
  Map<ObjectInfo *, SmallVector<Candidate *>> bases = {};
  for (auto &candidate : candidates)
    for (const auto &[base, _] : candidate.equiv)
      bases[base].push_back(&candidate);

  // Create a global for each of the bases, and update the mapping.
  Map<ObjectInfo *, Pair<llvm::GlobalVariable *, llvm::GlobalVariable *>>
      mappings = {};
  for (const auto &[base, base_candidates] : bases) {
    // Create globals for this base.
    auto &function =
        MEMOIR_SANITIZE(base->function(), "Base has no parent function");
    auto &module =
        MEMOIR_SANITIZE(function.getParent(), "Function has no parent module");

    auto &enc = create_global_ptr(module, "enc." + function.getName());
    mappings[base].first = &enc;

    auto &dec = create_global_ptr(module, "dec." + function.getName());
    mappings[base].second = &dec;

    // Record the global in each relevant candidate.
    for (auto *candidate : base_candidates) {
      candidate->encoder.global(*base, enc);
      candidate->decoder.global(*base, dec);
    }
  }

  // Link the caller-callee for each base.
  for (const auto &[base, globals] : mappings) {
    const auto &[enc, dec] = globals;

    // Non-argument bases will be handled later.
    auto *arg = dyn_cast<ArgObjectInfo>(base);
    if (not arg)
      continue;

    // Collect the callers for this function.
    auto &func = MEMOIR_SANITIZE(arg->function(), "Arg has no parent");

    auto callers = possible_callers(func);

    for (const auto &[call, incoming] : arg->incoming()) {
      // Get the globals for the caller base.
      const auto &[caller_enc, caller_dec] = mappings.at(incoming);
      store_mappings_for_base(call, enc, caller_enc, dec, caller_dec);
    }
  }
}

static llvm::AllocaInst &load_global_to_stack(llvm::GlobalVariable &global,
                                              Type &mapping_type,
                                              llvm::Function &func,
                                              const llvm::Twine &name = "") {

  MemOIRBuilder builder(func.getEntryBlock().getFirstNonPHI());

  auto *type = global.getValueType();

  // Create a stack variable.
  auto *var = builder.CreateAlloca(type, 0, name.concat(".local"));

  // Load the mapping.
  auto *val = builder.CreateLoad(type, &global, name);

  // Assert the type of the global.
  builder.CreateAssertTypeInst(val, mapping_type, name.concat(".type"));

  // Store the mapping to the stack variable.
  builder.CreateStore(val, var);

  return *var;
}

static void create_base_locals(Vector<Candidate> &candidates) {
  for (auto &candidate : candidates) {
    // Iterate over each object in the equivalence class, and give each
    // one a local, then load its base global into the local.

    // Handle encoders.
    auto &enc_type = candidate.encoder_type();
    for (const auto &[base, equiv] : candidate.equiv) {
      auto &enc = candidate.encoder.global(*base);
      for (auto *obj : equiv) {
        auto *func = obj->function();
        if (not func)
          continue;

        auto &var = load_global_to_stack(enc, enc_type, *func, "enc");

        candidate.encoder.local(*base, var);
      }
    }

    // Handle decoders.
    auto &dec_type = candidate.decoder_type();
    for (const auto &[base, equiv] : candidate.equiv) {
      auto &dec = candidate.decoder.global(*base);
      for (auto *obj : equiv) {
        auto *func = obj->function();
        if (not func)
          continue;

        auto &var = load_global_to_stack(dec, dec_type, *func, "dec");

        candidate.decoder.local(*base, var);
      }
    }
  }
}

void ProxyInsertion::prepare() {

  // Monomorphize the program, for candidate patterns.
  // monomorphize(this->candidates);

  // For each base in the candidate, insert a global for it.
  create_base_globals(this->candidates);

  // For each base global, create a local stack variable to hold it.
  create_base_locals(this->candidates);
}

} // namespace folio
