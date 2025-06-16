#include "memoir/ir/Builder.hpp"

#include "folio/ProxyInsertion.hpp"
#include "folio/Utilities.hpp"
#include "folio/Version.hpp"

using namespace llvm::memoir;

namespace folio {

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

  // For each argument, load the local value for this candidate and store it to
  // the temparg.
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
    // For recursive functions, we need to know if we should pass the current or
    // last iteration into the this call.

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

void version_function(llvm::Function &func, Vector<Version> &versions) {

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
  Set<llvm::CallBase *> possible_callers = {};
  for (auto &use : func.uses()) {
    auto *call = dyn_cast<llvm::CallBase>(use.getUser());

    // Check if the use may lead to an indirect call.
    if (not call) {
      possibly_unknown = true;

    } else if (auto *ret_phi = into<RetPHIInst>(call)) {
      if (&use != &ret_phi->getCalledOperandAsUse()) {
        possibly_unknown = true;
      }

    } else if (auto *fold = into<FoldInst>(call)) {
      if (&use != &fold->getBodyOperandAsUse()) {
        possibly_unknown = true;
      }

    } else if (&use != &call->getCalledOperandUse()) {
      possibly_unknown = true;

    } else {
      // Otherwise, we have a direct call to the function.
      possible_callers.insert(call);
    }
  }

  // If the address of this function was taken, it may be indirectly called.
  // The defunctionalize utility in memoir can fix this.
  MEMOIR_ASSERT(not possibly_unknown,
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

void ProxyInsertion::prepare() {

  auto &candidates = this->candidates;

  // Collect all abstract candidate arguments.
  Map<Context, Map<llvm::Argument *, Set<Candidate *>>>
      abstract_candidates = {};
  for (auto &candidate : candidates) {
    println("PREPARING ", candidate);

    // Find all argument redefinitions in the candidate.
    for (const auto *info : candidate) {
      for (const auto &[func, pair] : info->redefinitions) {
        const auto &[call, locals] = pair;
        Context ctx(*func, call);
        for (const auto &[base, redefs] : locals) {
          if (auto *arg = dyn_cast<llvm::Argument>(base)) {
            abstract_candidates[ctx][arg].insert(&candidate);
          }
        }
      }
    }

    // Create global variables for this candidate.
    auto &module = MEMOIR_SANITIZE(candidate.function().getParent(),
                                   "Function has no parent module!");
    if (candidate.build_encoder()) {
      candidate.encoder.global(create_global_ptr(module, "encoder"));
    }
    if (candidate.build_decoder()) {
      candidate.decoder.global(create_global_ptr(module, "decoder"));
    }
  }

  // Find all versions of the function that are needed.
  Map<llvm::Function *, Vector<Version>> function_versions = {};
  Map<Context, size_t> context_to_version = {};
  for (const auto &[ctx, locals] : abstract_candidates) {
    // Unpack the context.
    auto &func = ctx.function();
    auto *caller = ctx.caller();

    Version version(&func, caller, func.arg_size());

    println("FUNC: ", func.getName());
    for (const auto &[arg, candidates] : locals) {
      // TODO: Add handling for arguments that have more than one
      // context-sensitive candidate.
      if (candidates.size() > 1) {
        MEMOIR_UNREACHABLE("ARG ", *arg, " HAS MULTIPLE CANDIDATES!");
      }

      version[arg->getArgNo()] = *candidates.begin();
    }

    // See if this version matched any existing ones.
    bool matches = false;
    auto vi = 0;
    for (auto &other : function_versions[&func]) {
      if (pattern_matches(version, other)) {
        other.add_caller(caller, version);
        matches = true;
        break;
      }

      ++vi;
    }

    if (not matches) {
      function_versions[&func].push_back(version);
    }

    context_to_version[ctx] = vi;
  }

  // Version the function
  for (auto &[func, versions] : function_versions) {
    version_function(*func, versions);
  }

  // Update calls to the versioned function.
  for (const auto &[func, versions] : function_versions) {
    for (const auto &version : versions) {

      auto *call = version.call;

      if (call) {
        patch_call(*call,
                   func,
                   version,
                   version.encoder_args,
                   version.decoder_args);
      }

      for (auto &[call, args] : version.callers) {
        if (call) {
          patch_call(*call,
                     func,
                     args,
                     version.encoder_args,
                     version.decoder_args);
        }
      }
    }
  }
}

} // namespace folio
