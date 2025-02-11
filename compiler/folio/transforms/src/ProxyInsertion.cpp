#include "llvm/IR/Verifier.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "memoir/ir/Builder.hpp"
#include "memoir/lowering/Implementation.hpp"
#include "memoir/transforms/utilities/ReifyTempArgs.hpp"
#include "memoir/utility/Metadata.hpp"

#include "folio/transforms/ProxyInsertion.hpp"

using namespace llvm::memoir;

namespace folio {

ProxyInsertion::ProxyInsertion(llvm::Module &M) : M(M) {

  // Register the bit{map,set} implementations.
  Implementation::define(
      { Implementation( // bitmap<T, U>
            "bitmap",
            AssocType::get(TypeVariable::get(), TypeVariable::get())),

        Implementation( // bitset<T>
            "bitset",
            AssocType::get(TypeVariable::get(), VoidType::get())) });

  analyze();

  transform();

  for (auto &F : M) {
    if (not F.empty()) {
      if (llvm::verifyFunction(F, &llvm::errs())) {
        println(F);
        MEMOIR_UNREACHABLE("Failed to verify ", F.getName());
      }
    }
  }
  println("Verified module post-proxy insertion.");

  reify_tempargs(M);

  for (auto &F : M) {
    if (not F.empty()) {
      if (llvm::verifyFunction(F, &llvm::errs())) {
        println(F);
        MEMOIR_UNREACHABLE("Failed to verify ", F.getName());
      }
    }
  }
  println("Verified module post-temparg reify.");
}

void ProxyInsertion::gather_assoc_objects(AllocInst &alloc,
                                          Type &type,
                                          vector<unsigned> offsets) {

  if (auto *struct_type = dyn_cast<StructType>(&type)) {
    for (unsigned field = 0; field < struct_type->getNumFields(); ++field) {
      auto new_offsets = offsets;
      new_offsets.push_back(field);

      this->gather_assoc_objects(alloc,
                                 struct_type->getFieldType(field),
                                 new_offsets);
    }

  } else if (auto *collection_type = dyn_cast<CollectionType>(&type)) {
    auto &elem_type = collection_type->getElementType();

    // If this is an assoc, add the object information.
    if (isa<AssocType>(collection_type) and isa<VoidType>(&elem_type)) {
      allocations.push_back(ObjectInfo(&alloc, offsets));
    }

    // Recurse on the element.
    auto new_offsets = offsets;
    new_offsets.push_back(-1);

    this->gather_assoc_objects(alloc, elem_type, new_offsets);
  }

  return;
}

void ProxyInsertion::analyze() {
  auto &M = this->M;

  vector<ObjectInfo> objects = {};

  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (auto *alloc = into<AllocInst>(&I)) {
          // Gather all of the Assoc allocations.
          this->gather_assoc_objects(*alloc, alloc->getType());
        }
      }
    }
  }
}

namespace detail {

bool is_last_index(llvm::Use *use, AccessInst::index_op_iterator index_end) {
  return std::next(AccessInst::index_op_iterator(use)) == index_end;
}

void gather_redefinitions(llvm::Value &V, set<llvm::Value *> &redefinitions) {

  if (redefinitions.count(&V) > 0) {
    return;
  }

  redefinitions.insert(&V);

  for (auto &use : V.uses()) {
    auto *user = use.getUser();

    // Recurse on redefinitions.
    if (auto *phi = dyn_cast<llvm::PHINode>(user)) {
      gather_redefinitions(*user, redefinitions);

    } else if (auto *memoir_inst = into<MemOIRInst>(user)) {
      if (auto *update = dyn_cast<UpdateInst>(memoir_inst)) {
        if (&use == &update->getObjectAsUse()) {
          gather_redefinitions(*user, redefinitions);
        }

      } else if (isa<RetPHIInst>(memoir_inst) or isa<UsePHIInst>(memoir_inst)) {

        // Recurse on redefinitions.
        gather_redefinitions(*user, redefinitions);
      }

      // Gather variable if folded on, or recurse on closed argument.
      else if (auto *fold = into<FoldInst>(user)) {

        if (use == fold->getInitialAsUse()) {
          // Gather uses of the accumulator argument.
          gather_redefinitions(fold->getAccumulatorArgument(), redefinitions);

          // Gather uses of the resultant.
          gather_redefinitions(fold->getResult(), redefinitions);

        } else if (use == fold->getObjectAsUse()) {
          // Do nothing.

        } else {
          // Gather uses of the closed argument.
          gather_redefinitions(fold->getClosedArgument(use), redefinitions);
        }
      }

    } else if (auto *call = dyn_cast<llvm::CallBase>(user)) {
      auto &callee =
          MEMOIR_SANITIZE(call->getCalledFunction(),
                          "Found use of MEMOIR collection by indirect callee!");

      auto &arg =
          MEMOIR_SANITIZE(callee.getArg(use.getOperandNo()),
                          "No arguments in the callee matching this use!");

      gather_redefinitions(arg, redefinitions);
    }
  }

  return;
}

int32_t indices_match_offsets(AccessInst &access, vector<unsigned> &offsets) {
  auto index_it = access.index_operands_begin();
  auto index_ie = access.index_operands_end();

  auto *type = &access.getObjectType();

  auto offset_it = offsets.begin();
  for (; offset_it != offsets.end(); ++offset_it) {

    auto offset = *offset_it;

    // If we have reached the end of the index operands, there is no index
    // use.
    if (index_it == index_ie) {
      break;
    }

    if (auto *struct_type = dyn_cast<StructType>(type)) {

      auto &index_use = *index_it;
      auto &index_const =
          MEMOIR_SANITIZE(dyn_cast<llvm::ConstantInt>(index_use.get()),
                          "Field index is not statically known!");

      // If the offset doesn't match the field index, there is no index use.
      if (offset != index_const.getZExtValue()) {
        return -1;
      }

      // Get the inner type.
      type = &struct_type->getFieldType(offset);

    } else if (auto *collection_type = dyn_cast<CollectionType>(type)) {
      // Get the inner type.
      type = &collection_type->getElementType();
    }

    ++index_it;
  }

  // Return the number of offsets we matched.
  return std::distance(offsets.begin(), offset_it);
}

llvm::Use *get_index_use(AccessInst &access, vector<unsigned> &offsets) {
  auto offset_it = offsets.begin();
  auto offset_ie = offsets.end();

  auto index_it = access.index_operands_begin();
  auto index_ie = access.index_operands_end();

  auto *type = &access.getObjectType();

  for (auto offset : offsets) {

    // If we have reached the end of the index operands, there is no index
    // use.
    if (index_it == index_ie) {
      return nullptr;
    }

    if (auto *struct_type = dyn_cast<StructType>(type)) {

      auto &index_use = *index_it;
      auto &index_const =
          MEMOIR_SANITIZE(dyn_cast<llvm::ConstantInt>(index_use.get()),
                          "Field index is not statically known!");

      // If the offset doesn't match the field index, there is no index use.
      if (offset != index_const.getZExtValue()) {
        return index_ie;
      }

      // Get the inner type.
      type = &struct_type->getFieldType(offset);

    } else if (auto *collection_type = dyn_cast<CollectionType>(type)) {
      // Get the inner type.
      type = &collection_type->getElementType();
    }

    ++index_it;
  }

  // If we are at the end of the index operands, return NULL.
  if (index_it == index_ie) {
    return nullptr;
  }

  // Otherwise, fetch the index use and return it.
  auto *index_use = &*index_it;

  return index_it;
}

void gather_uses_to_proxy(llvm::Value &V,
                          vector<unsigned> &offsets,
                          set<llvm::Use *> &to_encode,
                          set<llvm::Use *> &to_decode,
                          set<llvm::Use *> &to_addkey) {
  // From a given collection, V, gather all uses that need to be either
  // encoded or decoded.
  for (auto &use : V.uses()) {
    auto *user = use.getUser();

    if (auto *fold = into<FoldInst>(user)) {

      if (use == fold->getObjectAsUse()) {

        // If we find an index use, encode it.
        if (auto *index_use = detail::get_index_use(*fold, offsets)) {
          to_encode.insert(index_use);

        } else {

          // If the offset is exactly equal to the keys being folded over,
          // decode the index argument of the body.
          auto distance = detail::indices_match_offsets(*fold, offsets);
          if (distance == -1) {
            // Do nothing.
          }

          // If the offset are fully exhausted, add uses of the index argument
          // to the set of uses to decode.
          else if (distance == int32_t(offsets.size())) {
            auto &index_arg = fold->getIndexArgument();
            for (auto &index_use : index_arg.uses()) {
              to_decode.insert(&index_use);
            }
          }

          // If the offsets are not fully exhausted, recurse on the value
          // argument.
          else if (distance > 0) {
            if (auto *elem_arg = fold->getElementArgument()) {
              vector<unsigned> nested_offsets(
                  std::next(offsets.begin(), distance),
                  offsets.end());
              gather_uses_to_proxy(*elem_arg,
                                   nested_offsets,
                                   to_encode,
                                   to_decode,
                                   to_addkey);
            }
          }
        }
      }

    } else if (auto *access = into<AccessInst>(user)) {

      // Find the index use for the given offset and mark it for decoding.
      if (auto *index_use = detail::get_index_use(*access, offsets)) {
        if (isa<InsertInst>(access)) {
          if (detail::is_last_index(index_use, access->index_operands_end())) {
            to_addkey.insert(index_use);
            continue;
          }
        }

        to_encode.insert(index_use);
      }
    }
  }

  return;
}

llvm::FunctionCallee create_addkey_function(llvm::Module &M,
                                            Type &key_type,
                                            bool build_encoder,
                                            Type *encoder_type,
                                            bool build_decoder,
                                            Type *decoder_type) {

  auto &context = M.getContext();
  auto &data_layout = M.getDataLayout();

  auto &size_type = Type::get_size_type(data_layout);

  auto *llvm_size_type = size_type.get_llvm_type(context);
  auto *llvm_ptr_type = llvm::PointerType::get(context, 0);
  auto *llvm_key_type = key_type.get_llvm_type(context);

  // Create the addkey functions for this proxy.
  vector<llvm::Type *> addkey_params = { llvm_key_type };
  if (build_encoder) {
    addkey_params.push_back(llvm_ptr_type);
  }
  if (build_decoder) {
    addkey_params.push_back(llvm_ptr_type);
  }
  auto *addkey_type =
      llvm::FunctionType::get(llvm_size_type, addkey_params, false);
  auto &addkey_function = MEMOIR_SANITIZE(
      llvm::Function::Create(addkey_type,
                             llvm::GlobalValue::LinkageTypes::InternalLinkage,
                             "proxy_addkey_",
                             M),
      "Failed to create LLVM function");

  auto arg_idx = 0;
  auto *key = addkey_function.getArg(arg_idx++);

  llvm::Argument *encoder = nullptr;
  if (build_encoder) {
    encoder = addkey_function.getArg(arg_idx++);
  }

  llvm::Argument *decoder = nullptr;
  if (build_decoder) {
    decoder = addkey_function.getArg(arg_idx++);
  }

  auto *ret_bb = llvm::BasicBlock::Create(context, "", &addkey_function);

  MemOIRBuilder builder(ret_bb);

  if (build_encoder) {
    builder.CreateAssertTypeInst(encoder, *encoder_type);
  }

  if (build_decoder) {
    builder.CreateAssertTypeInst(decoder, *decoder_type);
  }

  auto *has_key = &builder.CreateHasInst(encoder, key)->getCallInst();
  auto *phi = builder.CreatePHI(llvm_size_type, 2);
  llvm::PHINode *encoder_phi = nullptr;
  if (build_encoder) {
    encoder_phi = builder.CreatePHI(llvm_ptr_type, 2);
  }

  llvm::PHINode *decoder_phi = nullptr;
  if (build_decoder) {
    decoder_phi = builder.CreatePHI(llvm_ptr_type, 2);
  }

  auto *ret = builder.CreateRet(phi);

  llvm::Instruction *then_terminator, *else_terminator;
  llvm::SplitBlockAndInsertIfThenElse(has_key,
                                      phi,
                                      &then_terminator,
                                      &else_terminator);
  // if (not has(encoder, key)) {
  //   z1 = size(encoder)
  //   insert(size(encoder), encoder, key)
  // }
  auto *then_bb = then_terminator->getParent();
  builder.SetInsertPoint(then_terminator);
  auto *read_index =
      &builder.CreateReadInst(size_type, encoder, { key })->getCallInst();

  // else {
  //   z2 = read(encoder, key)
  // }
  auto *else_bb = else_terminator->getParent();
  builder.SetInsertPoint(else_terminator);
  llvm::Value *new_index = nullptr;
  llvm::Value *new_encoder = nullptr;
  if (encoder) {
    new_index = &builder.CreateSizeInst(encoder)->getCallInst();
    auto *insert = builder.CreateInsertInst(encoder, { key });
    new_encoder = &builder
                       .CreateWriteInst(size_type,
                                        new_index,
                                        &insert->getCallInst(),
                                        { key })
                       ->getCallInst();
  }

  llvm::Value *new_decoder = nullptr;
  if (decoder) {
    if (not new_index) {
      new_index = &builder.CreateSizeInst(decoder)->getCallInst();
    }
    auto *end = &builder.CreateEndInst()->getCallInst();
    auto *insert = builder.CreateInsertInst(encoder, { end });
    new_decoder = &builder
                       .CreateWriteInst(size_type,
                                        new_index,
                                        &insert->getCallInst(),
                                        { new_index })
                       ->getCallInst();
  }

  // Update the PHIs
  phi->addIncoming(read_index, then_bb);
  phi->addIncoming(new_index, else_bb);

  if (encoder_phi) {
    encoder_phi->addIncoming(encoder, then_bb);
    encoder_phi->addIncoming(new_encoder, else_bb);
    auto encoder_live_out = Metadata::get_or_add<LiveOutMetadata>(*encoder_phi);
    encoder_live_out.setArgNo(encoder->getArgNo());
  }

  if (decoder_phi) {
    decoder_phi->addIncoming(decoder, then_bb);
    decoder_phi->addIncoming(new_decoder, else_bb);
    auto decoder_live_out = Metadata::get_or_add<LiveOutMetadata>(*decoder_phi);
    decoder_live_out.setArgNo(decoder->getArgNo());
  }

  return llvm::FunctionCallee(&addkey_function);
}

void add_tempargs(map<llvm::Function *, llvm::Instruction *> &local_patches,
                  llvm::ArrayRef<set<llvm::Value *>> values_to_patch,
                  llvm::ArrayRef<set<llvm::Use *>> uses_to_patch,
                  llvm::Instruction &patch_with,
                  Type &patch_type) {

  println(patch_with);

  MemOIRBuilder builder(&patch_with);
  auto &context = builder.getContext();
  auto &module = builder.getModule();

  // Unpack the patch.
  auto *patch_func = patch_with.getFunction();
  auto *llvm_patch_type = patch_type.get_llvm_type(context);

  // Track the local patch for each function.
  local_patches[patch_func] = &patch_with;

  // Find the set of functions that need the patch.
  set<llvm::Function *> functions = {};
  for (auto &values : values_to_patch) {
    for (auto *val : values) {
      if (auto *arg = dyn_cast<llvm::Argument>(val)) {
        functions.insert(arg->getParent());
      } else if (auto *inst = dyn_cast<llvm::Instruction>(val)) {
        functions.insert(inst->getFunction());
      }
    }
  }

  for (auto &uses : uses_to_patch) {
    for (auto *use : uses) {
      if (auto *inst = dyn_cast<llvm::Instruction>(use->getUser())) {
        functions.insert(inst->getFunction());
      }
    }
  }

  // Determine the set of functions that we need to pass the proxy to.
  map<llvm::CallBase *, llvm::GlobalVariable *> calls_to_patch = {};
  map<FoldInst *, llvm::GlobalVariable *> folds_to_patch = {};
  for (auto *func : functions) {
    // Create the global variable.
    auto *global = new llvm::GlobalVariable(
        module,
        llvm_patch_type,
        /* isConstant? */ false,
        llvm::GlobalValue::LinkageTypes::InternalLinkage,
        llvm::Constant::getNullValue(llvm_patch_type),
        "temparg");

    // Create the load in the entry of the fold body.
    builder.SetInsertPoint(func->getEntryBlock().getFirstNonPHI());

    auto *load = builder.CreateLoad(llvm_patch_type, global);
    Metadata::get_or_add<TempArgumentMetadata>(*load);

    // Annotate the loaded value with type information.
    builder.CreateAssertTypeInst(load, patch_type);

    local_patches[func] = load;

    if (auto *single_use = func->getSingleUndroppableUse()) {
      auto *single_user = single_use->getUser();
      if (auto *fold = into<FoldInst>(single_user)) {
        folds_to_patch[fold] = global;
      }
    } else {
      for (auto &use : func->uses()) {
        if (auto *call = dyn_cast<llvm::CallBase>(use.getUser())) {
          calls_to_patch[call] = global;
        }
      }
    }
  }

  // Patch each of the folds by storing to the global before the operation.
  for (const auto &[fold, global] : folds_to_patch) {
    // Unpack.
    auto &inst = fold->getCallInst();
    auto *func = inst.getFunction();
    println(func->getName());

    // Create the store ahead of the fold.
    builder.SetInsertPoint(&inst);

    auto *local = local_patches[func];

    MEMOIR_ASSERT(local, "Failed to find the local patch in ", func->getName());

    auto *store = builder.CreateStore(local, global);
    Metadata::get_or_add<TempArgumentMetadata>(*store);
  }

  // TODO:
  // Patch each of the calls by storing to the global before the operation.
  for (const auto &[call, global] : calls_to_patch) {
    println("Need to patch ", *call);
  }

  return;
}

} // namespace detail

bool ProxyInsertion::transform() {

  bool modified = false;

  // Collect the set of redefinitions for each allocation involved.
  ordered_map<AllocInst *, set<llvm::Value *>> redefinitions = {};
  for (auto &info : this->allocations) {

    // Unpack the object information.
    auto *alloc = info.allocation;
    auto *type = &alloc->getType();

    // Get the nested object type.
    for (auto offset : info.offsets) {
      if (auto *struct_type = dyn_cast<StructType>(type)) {
        type = &struct_type->getFieldType(offset);
      } else if (auto *collection_type = dyn_cast<CollectionType>(type)) {
        type = &collection_type->getElementType();
      } else {
        MEMOIR_UNREACHABLE("Invalid offsets provided.");
      }
    }

    auto &assoc_type = MEMOIR_SANITIZE(

        dyn_cast<AssocType>(type),
        "Proxy insertion for non-assoc collection is unsupported");

    auto &key_type = assoc_type.getKeyType();
    auto &val_type = assoc_type.getValueType();

    // Gather redefinitions of the allocation.
    detail::gather_redefinitions(alloc->getCallInst(), redefinitions[alloc]);

    // Collect the set of uses that need to be updated to use the proxy space.
    // We will separate these into uses that need the be encoded/decoded.
    set<llvm::Use *> to_encode = {};
    set<llvm::Use *> to_decode = {};
    set<llvm::Use *> to_addkey = {};

    // Iterate over all uses of redefinitions to find any uses that need
    // updated.
    for (auto *redef : redefinitions[alloc]) {
      detail::gather_uses_to_proxy(*redef,
                                   info.offsets,
                                   to_encode,
                                   to_decode,
                                   to_addkey);
    }

    println("  before trimming:");
    println("  ", to_encode.size(), " uses to encode");
    println("  ", to_decode.size(), " uses to decode");
    println("  ", to_addkey.size(), " uses to addkey");

    // Trim uses that dont need to be decoded because they are only used to
    // compare against other values that need to be decoded.
    set<llvm::Use *> trim_to_decode = {};
    for (auto *use : to_decode) {
      auto *user = use->getUser();

      for (auto *other_use : to_decode) {
        if (use == other_use) {
          continue;
        }

        auto *other_user = other_use->getUser();
        if (user != other_user) {
          continue;
        }

        if (auto *cmp = dyn_cast<llvm::CmpInst>(user)) {
          if (cmp->isEquality()) {
            trim_to_decode.insert(use);
            trim_to_decode.insert(other_use);
          }
        }
      }
    }

    // Trim uses that dont need to be encoded because they are produced by a use
    // that needs decoded.
    set<llvm::Use *> trim_to_encode = {};
    for (auto uses : { to_encode, to_addkey }) {
      for (auto *encodee : uses) {
        auto found = to_decode.find(encodee);
        if (found != to_decode.end()) {
          auto *decodee = *found;
          trim_to_encode.insert(encodee);
          trim_to_decode.insert(decodee);
        }
      }
    }

    // Erase the uses that we identified to trim.
    for (auto *use_to_trim : trim_to_decode) {
      to_decode.erase(use_to_trim);
    }
    for (auto *use_to_trim : trim_to_encode) {
      to_encode.erase(use_to_trim);
      to_addkey.erase(use_to_trim);
    }

    println("  after trimming:");
    println("  ", to_encode.size(), " uses to encode");
    for (auto *use : to_encode) {
      println("    ", *use->get(), " in ", *use->getUser());
    }

    println("  ", to_decode.size(), " uses to decode");
    for (auto *use : to_decode) {
      println("    ", *use->get(), " in ", *use->getUser());
    }

    println("  ", to_addkey.size(), " uses to addkey");
    for (auto *use : to_addkey) {
      println("    ", *use->get(), " in ", *use->getUser());
    }

    // Find the construction point for the encoder and decoder.
    auto *construction_point = &alloc->getCallInst();

    // Fetch LLVM context.
    auto &context = construction_point->getContext();
    auto &module = MEMOIR_SANITIZE(construction_point->getModule(),
                                   "Construction point has no module.");
    auto &data_layout = module.getDataLayout();

    // Allocate the proxy.
    MemOIRBuilder builder(construction_point);

    auto &size_type = Type::get_size_type(data_layout);
    auto &llvm_size_type = *size_type.get_llvm_type(context);
    auto size_type_bitwidth = size_type.getBitWidth();

    // Determine which proxies we need.
    bool build_encoder = to_encode.size() > 0 or to_addkey.size() > 0;
    bool build_decoder = to_decode.size() > 0;

    llvm::Instruction *encoder = nullptr;
    Type *encoder_type = nullptr;
    if (build_encoder) {
      auto *encoder_alloc = builder.CreateAssocArrayAllocInst(key_type,
                                                              size_type,
                                                              "proxy.encode.");
      encoder = &encoder_alloc->getCallInst();
      encoder_type = &encoder_alloc->getType();
    }

    llvm::Instruction *decoder = nullptr;
    Type *decoder_type = nullptr;
    if (build_decoder) {
      auto *decoder_alloc =
          builder.CreateSequenceAllocInst(key_type, size_t(0), "proxy.decode.");
      decoder = &decoder_alloc->getCallInst();
      decoder_type = &decoder_alloc->getType();
    }

    // Make the proxy available at all uses.
    map<llvm::Function *, llvm::Instruction *> function_to_encoder = {};
    if (build_encoder) {
      detail::add_tempargs(function_to_encoder,
                           { redefinitions[alloc] },
                           { to_encode, to_addkey },
                           *encoder,
                           *encoder_type);
    }

    map<llvm::Function *, llvm::Instruction *> function_to_decoder = {};
    if (build_decoder) {
      detail::add_tempargs(function_to_decoder,
                           { redefinitions[alloc] },
                           { to_decode, to_addkey },
                           *decoder,
                           *decoder_type);
    }

    // Create the addkey function.
    auto addkey_callee = detail::create_addkey_function(this->M,
                                                        key_type,
                                                        build_encoder,
                                                        encoder_type,
                                                        build_decoder,
                                                        decoder_type);

    // For each of the uses to encode, encode them.
    for (auto *use : to_encode) {
      println("Encoding use ", *use->getUser());

      // Unpack the use.
      auto *used = use->get();

      // Find the use's program point.
      auto *user_as_inst = dyn_cast<llvm::Instruction>(use->getUser());
      auto *user_bb = user_as_inst->getParent();
      auto *user_func = user_bb->getParent();

      // TODO: Extend this to re-use en/decodings that dominate the given
      // program point.

      // Fetch the encoder for this function.
      auto *encoder = function_to_encoder.at(user_func);

      // Compute the insertion point.
      llvm::Instruction *program_point = nullptr;
      if (user_as_inst) {
        if (auto *phi = dyn_cast<llvm::PHINode>(user_as_inst)) {
          MEMOIR_UNREACHABLE("En/decoding uses by PHI is unhandled.");
        } else {
          program_point = user_as_inst;
        }

      } else {
        MEMOIR_UNREACHABLE("Failed to find a point to encode the value!");
      }

      builder.SetInsertPoint(program_point);

      // Handle has operations separately.
      if (auto *has = into<HasInst>(user_as_inst)) {
        if (detail::is_last_index(use, has->index_operands_end())) {
          // Construct an if-else block.
          // if (has(encoder, key))
          //   i = read(encoder, key)
          //   h = has(collection, i)
          // h' = PHI(h, false)
          auto *cond = &builder.CreateHasInst(encoder, used)->getCallInst();
          auto *phi = builder.CreatePHI(cond->getType(), 2);

          auto *then_terminator =
              llvm::SplitBlockAndInsertIfThen(cond,
                                              phi,
                                              /* unreachable? */ false);

          user_as_inst->moveBefore(then_terminator);

          builder.SetInsertPoint(user_as_inst);

          auto *encoded =
              &builder.CreateReadInst(size_type, encoder, used)->getCallInst();

          println("set ", *use->get(), " to ", *encoded);
          use->set(encoded);

          user_as_inst->replaceAllUsesWith(phi);

          auto *then_bb = then_terminator->getParent();
          phi->addIncoming(user_as_inst, then_bb);

          auto *else_bb = cond->getParent();
          auto *false_constant = llvm::ConstantInt::getFalse(context);
          phi->addIncoming(false_constant, else_bb);

          println(*then_bb);
          println(*phi->getParent());

          continue;
        }
      }

      // In the common case, read the encoded value and update the use with it.
      auto *encoded =
          &builder.CreateReadInst(size_type, encoder, used)->getCallInst();

      println("set ", *use->get(), " to ", *encoded);
      use->set(encoded);
    }

    // For each of the uses to encode, encode them.
    for (auto *use : to_addkey) {
      println("Adding key for use ", *use->getUser());

      // Unpack the use.
      auto *used = use->get();

      // Find the use's program point.
      auto *user_as_inst = dyn_cast<llvm::Instruction>(use->getUser());
      auto *user_bb = user_as_inst->getParent();
      auto *user_func = user_bb->getParent();

      // TODO: Extend this to re-use en/decodings that dominate the given
      // program point.

      // Compute the insertion point.
      llvm::Instruction *program_point = nullptr;
      if (user_as_inst) {
        if (auto *phi = dyn_cast<llvm::PHINode>(user_as_inst)) {
          MEMOIR_UNREACHABLE("En/decoding uses by PHI is unhandled.");
        } else {
          program_point = user_as_inst;
        }

      } else {
        MEMOIR_UNREACHABLE("Failed to find a point to encode the value!");
      }

      builder.SetInsertPoint(program_point);

      vector<llvm::Value *> args = { use->get() };
      if (build_encoder) {
        auto *encoder = function_to_encoder.at(user_func);
        args.push_back(encoder);
      }
      if (build_decoder) {
        auto *decoder = function_to_decoder.at(user_func);
        args.push_back(decoder);
      }

      auto *encoded = builder.CreateCall(addkey_callee, args);

      println("set ", *use->get(), " to ", *encoded);

      use->set(encoded);
    }

    for (auto *use : to_decode) {
      println("Decoding use ", *use->getUser());

      // Unpack the use.
      auto *used = use->get();

      // Find the use's program point.
      auto *user_as_inst = dyn_cast<llvm::Instruction>(use->getUser());
      auto *user_bb = user_as_inst->getParent();
      auto *user_func = user_bb->getParent();

      // TODO: Extend this to re-use en/decodings that dominate the given
      // program point.

      // Fetch the decoder for this function.
      auto *decoder = function_to_decoder.at(user_func);

      // Compute the insertion point.
      llvm::Instruction *program_point = nullptr;
      if (user_as_inst) {
        if (auto *phi = dyn_cast<llvm::PHINode>(user_as_inst)) {
          auto *incoming_block = phi->getIncomingBlock(*use);
          // TODO: this may be unsound if the terminator is conditional.
          program_point = incoming_block->getTerminator();
        } else {
          program_point = user_as_inst;
        }

      } else {
        MEMOIR_UNREACHABLE("Failed to find a point to decode the value!");
      }

      builder.SetInsertPoint(program_point);

      // Then, read the value from the decoder.
      auto *decoded =
          &builder.CreateReadInst(key_type, decoder, { use->get() })
               ->getCallInst();

      println("set ", *use->get(), " to ", *decoded);

      use->set(decoded);
    }

    // Set the selection of the collection.

    // Determine _where_ to attach the selection.
    auto selection = Metadata::get_or_add<SelectionMetadata>(*alloc);
    unsigned selection_index = 0;
    {
      auto *type = &alloc->getType();
      for (auto offset : info.offsets) {
        if (auto *struct_type = dyn_cast<StructType>(type)) {
          auto &field_type = struct_type->getFieldType(offset);
          type = &field_type;

          selection =
              Metadata::get_or_add<SelectionMetadata>(*struct_type, offset);
          selection_index = 0;

        } else if (auto *collection_type = dyn_cast<CollectionType>(type)) {
          auto &elem_type = collection_type->getElementType();
          type = &elem_type;

          ++selection_index;
        }
      }
    }

    if (isa<VoidType>(&val_type)) {
      selection.setImplementation("bitset", selection_index);
    } else {
      selection.setImplementation("bitmap", selection_index);
    }

    // Update the type of the allocations.
    /*
    for (auto *alloc : this->allocations) {
      // Update the key type of assoc allocations.
      if (auto *assoc_alloc = dyn_cast<AssocAllocInst>(alloc)) {
        // Create a builder at the allocation.
        MemOIRBuilder builder(*assoc_alloc);

        println("orig alloc type: ", assoc_alloc->getCollectionType());

        // Construct a memoir usize type.
        auto *size_type_value = &builder.CreateSizeTypeInst()->getCallInst();

        // Replace the key type with the size type.
        auto &key_type_use = assoc_alloc->getKeyOperandAsUse();
        key_type_use.set(size_type_value);

        // Get the allocation type.
        auto &alloc_type = assoc_alloc->getCollectionType();
        println("new alloc type: ", alloc_type);

        // Update any type annotations as well.
        for (auto *redef : redefinitions[alloc]) {
          for (auto &use : redef->uses()) {
            auto *user = use.getUser();
            if (auto *assert_type = into<AssertTypeInst>(user)) {
              // Construct the allocation type.
              builder.SetInsertPoint(&assert_type->getCallInst());
              auto *alloc_type_value =
                  &builder.CreateTypeInst(alloc_type)->getCallInst();

              // Replace the type operand with the allocation type value.
              auto &type_use = assert_type->getTypeOperandAsUse();
              type_use.set(alloc_type_value);
            }
          }
        }
      }
    }
    */
  }

  return modified;
}

} // namespace folio
