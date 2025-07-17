#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "memoir/support/Casting.hpp"
#include "memoir/utility/Metadata.hpp"

#include "folio/Candidate.hpp"
#include "folio/RedundantTranslations.hpp"
#include "folio/Utilities.hpp"
#include "folio/WeakenUses.hpp"

using namespace llvm::memoir;

namespace folio {

static llvm::cl::opt<bool> disable_use_weakening(
    "disable-use-weakening",
    llvm::cl::desc("Disable weakening uses"),
    llvm::cl::init(true));

llvm::Module &Candidate::module() const {
  return MEMOIR_SANITIZE(this->front()->allocation->getModule(),
                         "Object in candidate has no parent module!");
}

llvm::Function &Candidate::function() const {
  return MEMOIR_SANITIZE(this->front()->allocation->getFunction(),
                         "Object in candidate has no parent function!");
}

llvm::memoir::Type &Candidate::key_type() const {
  // Unpack the first object information.
  auto *first_info = this->front();
  auto *alloc = first_info->allocation;
  auto *type = &alloc->getType();

  // Get the nested object type.
  for (auto offset : first_info->offsets) {
    if (auto *tuple_type = dyn_cast<TupleType>(type)) {
      type = &tuple_type->getFieldType(offset);
    } else if (auto *collection_type = dyn_cast<CollectionType>(type)) {
      type = &collection_type->getElementType();
    } else {
      MEMOIR_UNREACHABLE("Invalid offsets provided.");
    }
  }

  auto &assoc_type = MEMOIR_SANITIZE(
      dyn_cast<AssocType>(type),
      "Proxy insertion for non-assoc collection is unsupported");

  return assoc_type.getKeyType();
}

llvm::memoir::Type &Candidate::encoder_type() const {
  auto &data_layout = this->module().getDataLayout();
  auto &size_type = Type::get_size_type(data_layout);
  return AssocType::get(this->key_type(), size_type);
}

llvm::memoir::Type &Candidate::decoder_type() const {
  return SequenceType::get(this->key_type());
}

static void print_uses(const Vector<llvm::Use *> &uses) {
  for (auto *use : uses) {
    println("    ", pretty_use(*use));
  }
}

static void print_uses(const Set<llvm::Use *> &uses) {
  for (auto *use : uses) {
    println("    ", pretty_use(*use));
  }
}

static void print_uses(const Set<llvm::Use *> &to_encode,
                       const Set<llvm::Use *> &to_decode,
                       const Set<llvm::Use *> &to_addkey) {
  println("    ", to_encode.size(), " USES TO ENCODE ");
  print_uses(to_encode);
  println();
  println("    ", to_decode.size(), " USES TO DECODE ");
  print_uses(to_decode);
  println();
  println("    ", to_addkey.size(), " USES TO ADDKEY ");
  print_uses(to_addkey);
  println();
}

static void print_uses(const LocalMap<Set<llvm::Use *>> &to_encode,
                       const LocalMap<Set<llvm::Use *>> &to_decode,
                       const LocalMap<Set<llvm::Use *>> &to_addkey) {
  println("TO ENCODE");
  for (const auto &[base, uses] : to_encode) {
    println(" BASE ", *base);
    print_uses(uses);
  }

  println("TO DECODE");
  for (const auto &[base, uses] : to_decode) {
    println(" BASE ", *base);
    print_uses(uses);
  }

  println("TO ADDKEY");
  for (const auto &[base, uses] : to_addkey) {
    println(" BASE ", *base);
    print_uses(uses);
  }

  return;
}

void Candidate::gather_uses(
    Map<llvm::Function *, LocalMap<Set<llvm::Value *>>> &encoded,
    LocalMap<Set<llvm::Use *>> &to_decode,
    LocalMap<Set<llvm::Use *>> &to_encode,
    LocalMap<Set<llvm::Use *>> &to_addkey) const {

  for (auto *info : *this) {
    for (const auto &[func, values] : info->encoded) {
      for (auto *val : values) {
        auto *base = info->encoded_base.at(val);
        encoded[func][base].insert(val);
      }
    }

    for (const auto &[func, uses] : info->to_encode) {
      for (auto *use : uses) {
        auto *base = info->to_encode_base.at(use);
        to_encode[base].insert(use);
      }
    }

    for (const auto &[func, uses] : info->to_addkey) {
      for (auto *use : uses) {
        auto *base = info->to_addkey_base.at(use);
        to_addkey[base].insert(use);
      }
    }
  }

  // Perform a forward analysis on the encoded values.
  // TODO: updateme.
  // forward_analysis(encoded);

  // Collect the set of uses to decode.
  for (const auto &[func, base_to_values] : encoded) {
    for (const auto &[base, values] : base_to_values) {
      for (auto *val : values) {
        for (auto &use : val->uses()) {
          auto *user = use.getUser();

          // If the user is a PHI/Select/Fold _and_ is also encoded, we don't
          // need to decode it.
          if (isa<llvm::PHINode>(user) or isa<llvm::SelectInst>(user)) {
            if (encoded[func].count(user)) {
              continue;
            }
          } else if (auto *fold = into<FoldInst>(user)) {
            auto *arg = (&use == &fold->getInitialAsUse())
                            ? &fold->getAccumulatorArgument()
                            : fold->getClosedArgument(use);
            if (encoded[&fold->getBody()].count(arg)) {
              continue;
            }
          } else if (auto *ret = dyn_cast<llvm::ReturnInst>(user)) {
            if (auto *func = ret->getFunction()) {
              if (auto *fold = FoldInst::get_single_fold(*func)) {
                auto *caller = fold->getFunction();
                auto &result = fold->getResult();
                // If the result of the fold is encoded, we don't decode here.
                if (encoded[caller].count(&result)) {
                  continue;
                }
              }
            }
          } else if (auto *call = dyn_cast<llvm::CallBase>(user)) {
            // TODO
          }

          // If the user is not an encoded propagator, we need to decode this
          // use.
          to_decode[base].insert(&use);
        }
      }
    }
  }

  return;
}

void Candidate::optimize(
    std::function<llvm::DominatorTree &(llvm::Function &)> get_domtree,
    std::function<BoundsCheckResult &(llvm::Function &)> get_bounds_checks) {

  // Collect all of the uses that need to be handled.
  this->gather_uses(this->encoded_values,
                    this->to_decode,
                    this->to_encode,
                    this->to_addkey);

  println("  FOUND USES");
  print_uses(to_encode, to_decode, to_addkey);

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

  eliminate_redundant_translations(to_decode, to_encode, to_addkey);

  println("  TRIMMED USES:");
  print_uses(to_encode, to_decode, to_addkey);

#if 0
  coalesce(decoded, to_decode, get_domtree);
  coalesce(encoded, to_encode, get_domtree);
  coalesce(added, to_addkey, get_domtree);

  // Report the coalescing.
  println("  AFTER COALESCING:");
  println("    USES TO ENCODE ", encoded.size());
  for (const auto &uses : encoded) {
    println("      COALESCED ", uses.value(), " ", *uses.base());
    print_uses(uses);
  }
  println("    USES TO DECODE ", decoded.size());
  for (const auto &uses : decoded) {
    println("      COALESCED ", uses.value(), " ", *uses.base());
    print_uses(uses);
  }
  println("    USES TO ADDKEY ", added.size());
  for (const auto &uses : added) {
    println("      COALESCED ", uses.value(), " ", *uses.base());
    print_uses(uses);
  }
#endif
}

bool Candidate::build_encoder() const {
  return this->to_encode.size() > 0 or this->to_addkey.size() > 0;
}

bool Candidate::build_decoder() const {
  return this->to_decode.size() > 0;
}

llvm::Instruction &Candidate::construction_point(
    llvm::DominatorTree &domtree) const {
  // Find the construction point for the encoder and decoder.
  llvm::Instruction *construction_point =
      &this->front()->allocation->getCallInst();

  // Find a point that dominates all of the object allocations.
  for (const auto *other : *this) {
    auto *other_alloc = other->allocation;
    auto &other_inst = other_alloc->getCallInst();

    construction_point =
        domtree.findNearestCommonDominator(construction_point, &other_inst);
  }

  return MEMOIR_SANITIZE(
      construction_point,
      "Failed to find a construction point for the candidate!");
}

static llvm::Function &create_addkey_function(llvm::Module &M,
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
  Vector<llvm::Type *> addkey_params = { llvm_key_type };
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

  // Unpack the function arguments.
  auto arg_idx = 0;
  auto *key = addkey_function.getArg(arg_idx++);

  llvm::Value *encoder_inout = NULL;
  if (build_encoder) {
    encoder_inout = addkey_function.getArg(arg_idx++);
  }

  llvm::Value *decoder_inout = NULL;
  if (build_decoder) {
    decoder_inout = addkey_function.getArg(arg_idx++);
  }

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

  return addkey_function;
}

llvm::FunctionCallee Candidate::addkey_callee() {
  if (this->addkey_function == NULL) {
    this->addkey_function = &create_addkey_function(this->module(),
                                                    this->key_type(),
                                                    this->build_encoder(),
                                                    &this->encoder_type(),
                                                    this->build_decoder(),
                                                    &this->decoder_type());
  }

  return llvm::FunctionCallee(this->addkey_function);
}

static Pair<llvm::Value *, llvm::Type *> get_ptr(Mapping &mapping,
                                                 llvm::Value &value,
                                                 llvm::Value *base) {
  if (parent_function(value) == parent_function(*base)) {
    auto &local = mapping.local(base);
    return make_pair(&local, local.getAllocatedType());
  }

  auto &global = mapping.global(base);
  return make_pair(&global, global.getValueType());
}

static llvm::Value *fetch_mapping(Mapping &mapping,
                                  Type &mapping_type,
                                  MemOIRBuilder &builder,
                                  llvm::Value &value,
                                  llvm::Value *base,
                                  const llvm::Twine &name = "") {
  auto [ptr, type] = get_ptr(mapping, value, base);
  auto *val = builder.CreateLoad(type, ptr, name);
  builder.CreateAssertTypeInst(val, mapping_type, name.concat(".type"));

  return val;
}

static llvm::Value *fetch_encoder(Candidate &candidate,
                                  MemOIRBuilder &builder,
                                  llvm::Value &value,
                                  llvm::Value *base) {
  return fetch_mapping(candidate.encoder,
                       candidate.encoder_type(),
                       builder,
                       value,
                       base,
                       "enc");
}

static llvm::Value *fetch_decoder(Candidate &candidate,
                                  MemOIRBuilder &builder,
                                  llvm::Value &value,
                                  llvm::Value *base) {
  return fetch_mapping(candidate.decoder,
                       candidate.decoder_type(),
                       builder,
                       value,
                       base,
                       "dec");
}

#ifndef PRINT_VALUES
#  define PRINT_VALUES 0
#endif

llvm::Instruction &Candidate::has_value(MemOIRBuilder &builder,
                                        llvm::Value &value,
                                        llvm::Value *base) {
#if PRINT_VALUES
  builder.CreateErrorf("HAS %u\n", { &value });
#endif

  auto *enc = fetch_encoder(*this, builder, value, base);
  return builder.CreateHasInst(enc, { &value })->getCallInst();
};

llvm::Value &Candidate::decode_value(MemOIRBuilder &builder,
                                     llvm::Value &value,
                                     llvm::Value *base) {
#if PRINT_VALUES
  builder.CreateErrorf("DEC %lu ", { &value });
#endif

  auto *dec = fetch_decoder(*this, builder, value, base);
  auto &decoded =
      builder.CreateReadInst(this->key_type(), dec, { &value })->asValue();

#if PRINT_VALUES
  builder.CreateErrorf("= %u\n", { &decoded });
#endif

  return decoded;
};

llvm::Value &Candidate::encode_value(MemOIRBuilder &builder,
                                     llvm::Value &value,
                                     llvm::Value *base) {
#if PRINT_VALUES
  builder.CreateErrorf("ENC %u ", { &value });
#endif

  auto *enc = fetch_encoder(*this, builder, value, base);

  auto &size_type = Type::get_size_type(this->module().getDataLayout());
  auto &encoded = builder.CreateReadInst(size_type, enc, &value)->asValue();

#if PRINT_VALUES
  builder.CreateErrorf("= %lu\n", { &encoded });
#endif

  return encoded;
};

llvm::Value &Candidate::add_value(MemOIRBuilder &builder,
                                  llvm::Value &value,
                                  llvm::Value *base) {
#if PRINT_VALUES
  builder.CreateErrorf("ADD %u ", { &value });
#endif

  Vector<llvm::Value *> args = { &value };

  llvm::AllocaInst *enc_inout = NULL, *dec_inout = NULL;

  if (this->build_encoder()) {
    // Fetch the current state of the encoder.
    auto *enc = fetch_encoder(*this, builder, value, base);

    // Create an inout stack variable and initialize it.
    enc_inout = builder.CreateLocal(builder.getPtrTy());
    builder.CreateLifetimeStart(enc_inout);
    builder.CreateStore(enc, enc_inout);

    // Pass the inout stack variable to the function.
    args.push_back(enc_inout);
  }
  if (this->build_decoder()) {
    // Fetch the current state of the decoder.
    auto *dec = fetch_decoder(*this, builder, value, base);

    // Create an inout stack variable and initialize it.
    dec_inout = builder.CreateLocal(builder.getPtrTy());
    builder.CreateLifetimeStart(dec_inout);
    builder.CreateStore(dec, dec_inout);

    // Pass the inout stack variable to the function.
    args.push_back(dec_inout);
  }

  // Call the addkey function.
  auto &encoded =
      MEMOIR_SANITIZE(builder.CreateCall(this->addkey_callee(), args),
                      "Failed to create call to addkey!");

  // Load the inout results.
  if (enc_inout) {
    auto *enc = builder.CreateLoad(builder.getPtrTy(), enc_inout);
    builder.CreateLifetimeEnd(enc_inout);
    auto [enc_state, enc_type] = get_ptr(this->encoder, value, base);
    builder.CreateStore(enc, enc_state);
  }
  if (dec_inout) {
    auto *dec = builder.CreateLoad(builder.getPtrTy(), dec_inout);
    builder.CreateLifetimeEnd(dec_inout);
    auto [dec_state, dec_type] = get_ptr(this->decoder, value, base);
    builder.CreateStore(dec, dec_state);
  }

#if PRINT_VALUES
  builder.CreateErrorf("= %lu\n", { &encoded });
#endif

  return encoded;
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                              const Candidate &candidate) {
  os << "CANDIDATE: ";
  for (const auto *info : candidate) {
    os << "\n  " << *info;
  }
  return os;
}

} // namespace folio
