#include "memoir/ir/Builder.hpp"
#include "memoir/ir/CallGraph.hpp"
#include "memoir/support/SortedVector.hpp"

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
    const Candidate::Mapping &mapping) {

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

static void gather_bases(Set<llvm::Value *> &bases,
                         const Vector<CoalescedUses> &uses) {
  for (const auto &use : uses) {
    bases.insert(use.base());
  }
}

static void create_base_globals(Vector<Candidate> &candidates) {

  for (auto &candidate : candidates) {
    // Gather the set of bases from the uses.
    Set<llvm::Value *> bases = {};
    gather_bases(bases, candidate.decoded);
    gather_bases(bases, candidate.encoded);
    gather_bases(bases, candidate.added);

    // For each base, create a global.
    for (auto *base : bases) {
      if (candidate.build_encoder()) {
        auto &global = create_global_ptr(candidate.module(), "enc.");
        candidate.encoder.global(base, global);
      }

      if (candidate.build_decoder()) {
        auto &global = create_global_ptr(candidate.module(), "dec.");
        candidate.decoder.global(base, global);
      }
    }
  }
}

static llvm::Value *find_base_of(llvm::Value &value,
                                 const Vector<Candidate> &candidates) {

  auto *func = parent_function(value);

  println("FIND BASE OF ", value);

  llvm::Value *found_base = NULL;
  for (const auto &candidate : candidates) {
    println("SEARCHING ", candidate);

    for (auto *info : candidate) {
      // Search the redefinitions for the value.
      auto [lo, hi] = info->redefinitions.equal_range(func);
      for (auto it = lo; it != hi; ++it) {
        for (const auto &[base, locals] : it->second.second) {
          println("  POSSIBLE BASE ", *base);

          bool found = false;
          for (const auto &local : locals) {
            println("    LOCAL ", local.value());
            if (&local.value() == &value) {
              found = true;
              break;
            }
          }

          if (found) {
            print("  PRESENT ");
            if (not found_base) {
              println("FIRST!");
              found_base = base;
            } else if (found_base != base) {
              println("MISMATCH!");
              MEMOIR_UNREACHABLE("Found base ",
                                 *found_base,
                                 " does not match base ",
                                 *base);
            } else {
              println("MATCHED!");
            }
          } else {
            println("  NOT PRESENT!");
          }
        }
      }
    }
  }

  return found_base;
}

static void patch_mappings(Vector<Candidate> &candidates) {

  // Collect all of the argument bases.
  Set<llvm::Argument *> arguments = {};
  Map<llvm::Function *, List<SortedVector<llvm::CallBase *>>> versions;
  for (auto &candidate : candidates) {
    collect_versions(arguments, versions, candidates, candidate.encoder);
    collect_versions(arguments, versions, candidates, candidate.decoder);
  }

  // Clean up empty and un-needed versions.
  cleanup_versions(versions);

  // Version each function.
  println("FOUND ", versions.size(), " FUNCTIONS TO VERSION");
  for (const auto &[func, func_versions] : versions) {
    println("FUNC ", func->getName());

    bool first = true, no_original = false;
    for (const auto &calls : func_versions) {

      if (not calls.empty()) {
        println(" VERSION");
        for (auto *call : calls) {
          println(*call);
        }
      }

      if (first) {
        if (not calls.empty()) {
          first = false;
        } else {
          no_original = true;
        }
        continue;
      }

      // Clone the function and update the argument mapping.
      MEMOIR_UNREACHABLE("Function versioning is unimplemented!");
    }
  }

  // Create a store to the global for each argument at the call site.
  for (auto *arg : arguments) {

    auto &func = MEMOIR_SANITIZE(arg->getParent(), "Argument has no parent");
    auto &module = MEMOIR_SANITIZE(func.getParent(), "Function has no module");

    // Create a global pointer for this argument.
    auto &enc_global = create_global_ptr(module, "enc.");

    // Collect the set of callers.
    auto callers = possible_callers(func);

    // For each caller, store to the relevant global.
    auto arg_no = arg->getArgNo();

    for (auto *call : callers) {
      auto *operand = call->getOperand(arg_no);

      // Get the base of this operand.
      auto &base = MEMOIR_SANITIZE(find_base_of(*operand, candidates),
                                   "No base for op ",
                                   arg_no,
                                   " in ",
                                   *call);

      println("ARG  ", *arg, " IN ", func.getName());
      println("BASE ", base);
      println("CALL ", *call);
    }
  }
}

template <typename T>
static bool pattern_matches(const Vector<T> &lhs, const Vector<T> &rhs) {
  Map<T, T> lmap = {}, rmap = {};

  if (lhs.size() != rhs.size()) {
    return false;
  }

  auto lit = lhs.begin(), lie = lhs.end();
  auto rit = rhs.begin(), rie = rhs.end();

  for (; lit != lie and rit != rie; ++lit, ++rit) {
    const auto &l = *lit;
    const auto &r = *rit;

    // If they are equal, continue.
    if (l == r) {
      continue;
    }

    // Otherwise see if they unify with the mapping.
    auto lfound = lmap.find(l);
    auto rfound = rmap.find(r);

    // If the values are in neither mapping, add them.
    if (lfound == lmap.end() and rfound == rmap.end()) {
      lmap[l] = r;
      rmap[r] = l;
      continue;
    }

    //  If the values are in both mappings, and they match, then continue.
    if (lfound != lmap.end() and rfound != rmap.end()) {
      if (lfound->second == r and rfound->second == l) {
        continue;
      }
    }

    // Otherwise, we found mismatch!
    return false;
  }

  return true;
}

static llvm::AllocaInst &load_global_to_stack(llvm::GlobalVariable &global,
                                              llvm::Function &func,
                                              const llvm::Twine &name = "") {

  // Create a stack variable in the function.
  auto &stack = create_stack_ptr(func, name.concat(".stack"));

  println("STORING ",
          global.getName(),
          " TO ",
          stack.getName(),
          " IN ",
          func.getName());

  // Fetch type information.
  auto *type = stack.getAllocatedType();

  // Load the global and store it into the stack slot.
  llvm::IRBuilder<> builder(stack.getNextNode());
  auto *load = builder.CreateLoad(type, &global);
  builder.CreateStore(load, &stack);

  return stack;
}

static Pair<llvm::GlobalVariable *, llvm::AllocaInst *> load_temparg_to_stack(
    llvm::Function &func,
    const llvm::Twine name = "") {
  auto &module =
      MEMOIR_SANITIZE(func.getParent(), "Function has no parent module.");

  // Create a temparg for the function.
  auto &temparg = create_global_ptr(module, name.concat(".temparg"));

  // Load the temparg into a new stack variable.
  auto &stack = load_global_to_stack(temparg, func, name);

  return make_pair(&temparg, &stack);
}

static void patch_call(llvm::CallBase &call,
                       llvm::Function *versioned_func,
                       const Vector<Candidate *> &candidate_args,
                       const Vector<llvm::GlobalVariable *> &encoder_args,
                       const Vector<llvm::GlobalVariable *> &decoder_args) {
  llvm::IRBuilder<> builder(&call);

  auto *func = call.getFunction();

  // Update the called function.
  call.setCalledFunction(versioned_func);

  // For each argument, load the local value for this candidate and store it
  // to the temparg.
  Set<Candidate *> seen = {};
  for (auto i = 0; i < candidate_args.size(); ++i) {
    auto *candidate = candidate_args[i];

    if (not candidate or seen.contains(candidate)) {
      continue;
    } else {
      seen.insert(candidate);
    }

    // Unpack the tempargs for this argument, if they exist.
    auto *enc_arg = encoder_args[i];
    auto *dec_arg = decoder_args[i];

    // TODO: this code needs to be moved to a later time.
    // For recursive functions, we need to know if we should pass the current
    // or last iteration into the this call.

    if (auto *enc_arg = encoder_args[i]) {
      if (auto *enc_local = candidate->encoder.local(*func)) {
        auto *type = enc_local->getAllocatedType();
        auto *load = builder.CreateLoad(type, enc_local);
        builder.CreateStore(load, enc_arg);
      }
    }

    if (auto *dec_arg = decoder_args[i]) {
      if (auto *dec_local = candidate->decoder.local(*func)) {
        auto *type = dec_local->getAllocatedType();
        auto *load = builder.CreateLoad(type, dec_local);
        builder.CreateStore(load, dec_arg);
      }
    }
  }
}

#if 0
  void version_function(llvm::Function &func, Vector<SortedVector<llvm::CallBase *>> &versions) {

  print("VERSION ", func.getName());

  // Ensure that we need only one version of the function, otherwise we need
  // to create fresh copies.
  if (versions.size() > 1) {
    println(" (", versions.size(), " VERSIONS)");
    MEMOIR_UNREACHABLE(
        "TODO: Function versioning for multiple candidates is not implemented.");
  } else {
    println(" (1 VERSION)");
  }

  // Collect the set of possible callers for this function.
  bool possibly_unknown = false;
  Set<llvm::CallBase *> possible_callers = possible_callers(func);

  // If the address of this function was taken, it may be indirectly called.
  // The defunctionalize utility in memoir can fix this.
  MEMOIR_ASSERT(not possible_callers.contains(unknown_caller()),
                "Possible indirect call to function! "
                "Employ defunctionalization to fix this.");

  // If these caller sets are not equal, or the function is externally
  // visible, we need to create separate clones.
  for (auto &version : versions) {
    possible_callers.erase(version.call);
    for (const auto &[call, _call_version] : version.callers) {
      possible_callers.erase(call);
    }
  }

  // If there are any remaining possible callers, or the function is
  // externally visible, we need to keep the original function around.
  bool need_original =
      (possible_callers.size() > 0 or func.hasExternalLinkage());
  if (possible_callers.size() > 0) {
    println("POSSIBLE CALLERS");
    for (auto *call : possible_callers) {
      println(*call);
    }
  }

  // Create a clone of the function with extra arguments for the
  // encoder/decoder.
  bool first = true;
  for (auto &version : versions) {

    // Fetch the parent module.
    auto &module =
        MEMOIR_SANITIZE(func.getParent(), "Function has no parent module");

    // For each version, create a clone of the function.
    version.func = &func;
    if (need_original or not first) {
      if (need_original) {
        println("NEED ORIGINAL");
      }

      // TODO: Create a clone of the function and deeply clone its CFG.
      MEMOIR_UNREACHABLE("Unimplemented, tell Tommy.");
    }

    // Collect the number of candidates passed into this function.
    auto idx = 0;
    Map<Candidate *, Pair<llvm::GlobalVariable *, llvm::GlobalVariable *>>
        in_version;
    for (auto *candidate : version) {
      if (not candidate) {
        continue;
      }

      // Check if we have seen this candidate yet.
      bool already_seen = in_version.contains(candidate);

      // Unpack the enc/dec tempargs to update.
      auto [encoder_arg, decoder_arg] = in_version[candidate];

      // If we've already seen the candidate, update this arguments tempargs.
      if (already_seen) {
        version.encoder_args[idx] = encoder_arg;
        version.decoder_args[idx] = decoder_arg;
        continue;
      }

      println();
      println("FUNC ", version.func->getName());
      println(*candidate);

      // Check if this argument is polymorphic.
      bool polymorphic = version.is_polymorphic(idx);
      println(polymorphic ? "POLYMORPHIC" : "MONOMORPHIC");

      // TODO: we want to be a bit smarter about this, should use that old cfg
      // analysis we cooked up to determine if a function needs the enc/dec.
      if (candidate->build_encoder()) {

        llvm::AllocaInst *local = NULL;
        if (polymorphic) {
          std::tie(encoder_arg, local) =
              load_temparg_to_stack(*version.func, "enc");

          version.encoder_args[idx] = encoder_arg;

        } else {
          local = &load_global_to_stack(candidate->encoder.global(),
                                        *version.func,
                                        "enc");
        }

        candidate->encoder.local(*version.func, *local);
      }

      if (candidate->build_decoder()) {
        llvm::AllocaInst *local = NULL;
        if (polymorphic) {
          std::tie(decoder_arg, local) =
              load_temparg_to_stack(*version.func, "dec");

          version.decoder_args[idx] = decoder_arg;

        } else {
          local = &load_global_to_stack(candidate->decoder.global(),
                                        *version.func,
                                        "dec");
        }

        candidate->decoder.local(*version.func, *local);
      }

      ++idx;
    }

    first = false;
  }
}
#endif

void ProxyInsertion::prepare() {

  // For each base in the candidate, insert a global for it.
  create_base_globals(this->candidates);

  // For each argument base, store the correct global to it at each call site.
  patch_mappings(this->candidates);
}

} // namespace folio
