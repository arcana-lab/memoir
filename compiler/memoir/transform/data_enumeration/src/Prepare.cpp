#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "memoir/ir/Builder.hpp"
#include "memoir/ir/CallGraph.hpp"
#include "memoir/support/SortedVector.hpp"

#include "DataEnumeration.hpp"
#include "Mapping.hpp"
#include "Utilities.hpp"
#include "Version.hpp"

using namespace memoir;

namespace memoir {

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
    debugln("ARG ", *arg, " IN ", arg->getParent()->getName());
    for (auto *call : calls) {
      debugln("  CALL ", *call);
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
    debugln("VERSIONS OF ", func->getName());
    for (const auto &version : func_versions) {
      if (version.empty()) {
        debugln(" NO ORIGINAL NEEDED");
        continue;
      }

      debugln(" VERSION");
      for (auto *call : version) {
        debugln("  ", *call);
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

#if 0  
static void create_base_locals(
    const Map<ObjectInfo *, SmallVector<ObjectInfo *>> &equiv,
    const Map<ObjectInfo *, Set<Candidate *>> &obj_candidates,
    Candidate &candidate,
    Type &mapping_type,
    Mapping &mapping,
    const llvm::Twine &name) {

  // Iterate over each object in the equivalence class, and give each
  // one a local, then load its base global into the local.
  for (auto *parent : candidate) {
    // Only create locals for parents.
    if (not equiv.contains(parent))
      continue;

    // Fetch the global.
    auto &global = mapping.global(*parent);
    auto *type = global.getValueType();

    for (auto *obj : equiv.at(parent)) {
      auto *func = obj->function();
      if (not func)
        continue;

      MemOIRBuilder builder(func->getEntryBlock().getFirstNonPHI());

      auto *local = builder.CreateAlloca(type, NULL, name.concat(".local"));

      mapping.local(*obj, *local);

      // If this is an argument, load the global mapping into the local.
      if (isa<ArgObjectInfo>(obj)) {
        // Load the mapping.
        auto *val = builder.CreateLoad(type, &global, name);

        // Assert the type of the global.
        builder.CreateAssertTypeInst(val, mapping_type, name.concat(".type"));

        // Store the mapping to the stack variable.
        builder.CreateStore(val, local);
      }
    }
  }
}

static void create_base_locals(
    Vector<Candidate> &candidates,
    const Map<ObjectInfo *, SmallVector<ObjectInfo *>> &equiv,
    const Map<ObjectInfo *, Set<Candidate *>> &obj_candidates) {

  for (auto &candidate : candidates) {
    create_base_locals(equiv,
                       obj_candidates,
                       candidate,
                       candidate.encoder_type(),
                       candidate.encoder,
                       "enc");
    create_base_locals(equiv,
                       obj_candidates,
                       candidate,
                       candidate.decoder_type(),
                       candidate.decoder,
                       "dec");
  }
}
#endif

static void print_mapping(const Mapping &mapping) {
  for (const auto &[obj, global] : mapping.globals()) {
    debugln("  ", *obj);
    debugln("  GLOBAL ", *global);
  }
}

static llvm::Function *create_addkey_function(llvm::Module &module,
                                              Type &key_type,
                                              bool build_encoder,
                                              Type *encoder_type,
                                              bool build_decoder,
                                              Type *decoder_type) {
  static int id;

  auto &context = module.getContext();
  auto &data_layout = module.getDataLayout();

  auto &size_type = Type::get_size_type(data_layout);

  auto *llvm_size_type = size_type.get_llvm_type(context);
  auto *llvm_ptr_type = llvm::PointerType::get(context, 0);
  auto *llvm_key_type = key_type.get_llvm_type(context);

  // Create the addkey functions for this enumeration.
  Vector<llvm::Type *> addkey_params = { llvm_key_type };
  if (build_encoder)
    addkey_params.push_back(llvm_ptr_type);
  if (build_decoder)
    addkey_params.push_back(llvm_ptr_type);

  auto *addkey_type =
      llvm::FunctionType::get(llvm_size_type, addkey_params, false);
  auto &addkey_function = MEMOIR_SANITIZE(
      llvm::Function::Create(addkey_type,
                             llvm::GlobalValue::LinkageTypes::InternalLinkage,
                             "enum_addkey_" + std::to_string(++id),
                             module),
      "Failed to create LLVM function");

  // Unpack the function arguments.
  auto arg_idx = 0;
  auto *key = addkey_function.getArg(arg_idx++);

  llvm::Value *encoder_inout = NULL;
  if (build_encoder)
    encoder_inout = addkey_function.getArg(arg_idx++);

  llvm::Value *decoder_inout = NULL;
  if (build_decoder)
    decoder_inout = addkey_function.getArg(arg_idx++);

  auto *ret_bb = llvm::BasicBlock::Create(context, "", &addkey_function);

  MemOIRBuilder builder(ret_bb);

  llvm::Value *encoder = NULL, *decoder = NULL;
  if (build_encoder) {
    encoder = builder.CreateLoad(llvm_ptr_type, encoder_inout, "enc.");
    builder.CreateAssertTypeInst(encoder, *encoder_type);
  }
  if (build_decoder) {
    decoder = builder.CreateLoad(llvm_ptr_type, decoder_inout, "dec.");
    builder.CreateAssertTypeInst(decoder, *decoder_type);
  }

  // Check if the encoder has the given key.
  auto *has_key = &builder.CreateHasInst(encoder, key)->getCallInst();

  // Construct the join block.
  auto *phi = builder.CreatePHI(llvm_size_type, 2);
  builder.CreateRet(phi);

  // Split the block and insert an if-then-else conditioned on the has-key.
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
  if (build_encoder) {
    new_index = &builder.CreateSizeInst(encoder)->getCallInst();
    auto *insert = builder.CreateInsertInst(encoder, { key });
    auto *write = builder.CreateWriteInst(size_type,
                                          new_index,
                                          &insert->asValue(),
                                          { key });
    builder.CreateStore(&write->asValue(), encoder_inout);
  }

  if (build_decoder) {
    if (not new_index) {
      new_index = &builder.CreateSizeInst(decoder)->getCallInst();
    }
    auto *end = &builder.CreateEndInst()->getCallInst();
    auto *insert = builder.CreateInsertInst(decoder, { end });
    auto *write = builder.CreateWriteInst(key_type,
                                          key,
                                          &insert->asValue(),
                                          { new_index });
    builder.CreateStore(&write->asValue(), decoder_inout);
  }

  // Update the PHIs
  MEMOIR_ASSERT(read_index, "Read index is NULL");
  phi->addIncoming(read_index, then_bb);
  MEMOIR_ASSERT(new_index, "New index is NULL");
  phi->addIncoming(new_index, else_bb);

  debugln(addkey_function);

  return &addkey_function;
}

void DataEnumeration::prepare() {

  // Monomorphize the program, for candidate patterns.
  // monomorphize(this->candidates);

  // Create a global for each of the bases, and update the mapping.
  int id = 0; // give each one a unique id.
  for (const auto &[base, children] : this->equiv) {
    // Create globals for this base.
    auto &function =
        MEMOIR_SANITIZE(base->function(), "Base has no parent function");
    auto &module =
        MEMOIR_SANITIZE(function.getParent(), "Function has no parent module");

    // Create the globals.
    auto &enc = create_global_ptr(module, "enc." + std::to_string(id));
    auto &dec = create_global_ptr(module, "dec." + std::to_string(id));
    ++id;

    // Record the global.
    this->to_transform[base].enc_global = &enc;
    this->to_transform[base].dec_global = &dec;
  }

  // Link the caller-callee for each base.
  for (const auto &[base, info] : this->to_transform) {
    // Non-argument bases will be handled later.
    auto *arg = dyn_cast<ArgObjectInfo>(base);
    if (not arg)
      continue;

    // Collect the callers for this function.
    auto &func = MEMOIR_SANITIZE(arg->function(), "Arg has no parent");

    auto callers = possible_callers(func);

    for (const auto &[call, incoming] : arg->incoming()) {
      // Get the globals for the caller base.
      auto *incoming_parent = this->unified.find(incoming);
      const auto &inc_globals = this->to_transform.at(incoming_parent);
      store_mappings_for_base(call,
                              info.enc_global,
                              inc_globals.enc_global,
                              info.dec_global,
                              inc_globals.dec_global);
    }
  }

#if 0
  // For each base global, create a local stack variable to hold it.
  create_base_locals(this->candidates, this->equiv, obj_candidates);
#endif

  // Record the encoder/decoder type in each transform info.
  for (auto &[base, info] : this->to_transform)
    for (auto &candidate : this->candidates) {
      auto *key_type = &candidate.key_type();
      auto *enc_type = &candidate.encoder_type();
      auto *dec_type = &candidate.decoder_type();

      for (auto *obj : candidate) {
        if (base == obj) {
          // Key type
          MEMOIR_ASSERT(not info.key_type or info.key_type == key_type,
                        "Mismatched key types");
          info.key_type = key_type;

          // Encoder type
          MEMOIR_ASSERT(not info.encoder_type or info.encoder_type == enc_type,
                        "Mismatched encoder types");
          info.encoder_type = enc_type;

          // Decoder type
          MEMOIR_ASSERT(not info.decoder_type or info.decoder_type == dec_type,
                        "Mismatched decoder types");
          info.decoder_type = dec_type;
        }
      }
    }

  // TODO: Determine if we need to build encoders/decoders.
  // Locally determine if each equiv class needs one.
  for (auto &[base, info] : this->to_transform) {
    info.build_encoder =
        not info.to_encode.empty() or not info.to_addkey.empty();
    info.build_decoder = not info.to_decode.empty();
  }

  // Propagate build information to arguments. Finally, back propagate from
  // arguments to base classes.
  Map<ObjectInfo *, SmallVector<ObjectInfo *>> outgoing;
  for (auto &[base, info] : this->to_transform) {
    for (auto *obj : this->equiv.at(base)) {
      if (auto *arg = dyn_cast<ArgObjectInfo>(obj)) {
        for (const auto &[call, incoming] : arg->incoming()) {
          auto *inc_base = this->unified.find(incoming);

          const auto &inc_info = this->to_transform.at(inc_base);
          info.build_encoder |= inc_info.build_encoder;
          info.build_decoder |= inc_info.build_decoder;

          outgoing[inc_base].push_back(base);
        }
      }
    }
  }

  // Back propagate from arguments to callers.
  for (const auto &[src, outs] : outgoing) {
    const auto &src_info = this->to_transform.at(src);
    for (auto *dst : outs) {
      auto &dst_info = this->to_transform.at(dst);
      dst_info.build_encoder |= src_info.build_encoder;
      dst_info.build_decoder |= src_info.build_decoder;
    }
  }

  for (auto &[base, info] : this->to_transform) {
    debugln("BASE ", *base);
    debug("  ENCODER ");
    if (info.build_encoder)
      debugln(*info.encoder_type);
    else
      debugln("NONE");
    debug("  DECODER ");
    if (info.build_decoder)
      debugln(*info.decoder_type);
    else
      debugln("NONE");
    debugln();
  }

  // Create the addkey function.
  for (auto &[base, info] : this->to_transform) {
    info.addkey_callee =
        llvm::FunctionCallee(create_addkey_function(this->module,
                                                    *info.key_type,
                                                    info.build_encoder,
                                                    info.encoder_type,
                                                    info.build_decoder,
                                                    info.decoder_type));
  }

  { // Debug print.
    debugln("=== PREPARED GLOBALS ===");
    for (auto &candidate : this->candidates) {
      debugln(" >> ENCODER << ");
      print_mapping(candidate.encoder);
      debugln(" >> DECODER << ");
      print_mapping(candidate.decoder);
      debugln();
    }
    debugln();
  }
}

} // namespace memoir
