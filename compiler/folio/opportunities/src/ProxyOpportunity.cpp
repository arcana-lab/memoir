#include <ranges>

#include "llvm/IR/Dominators.h"

#include "memoir/ir/Builder.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/PassUtils.hpp"
#include "memoir/utility/Metadata.hpp"

#include "folio/opportunities/ProxyOpportunity.hpp"
#include "folio/transforms/ProxyManager.hpp"

using namespace llvm::memoir;

namespace folio {

// ===========================
// ProxyOpportunity
template <>
std::string Opportunity::formulate<ProxyOpportunity>(FormulaEnvironment &env) {
  auto size_type = Type::get_size_type(env.module().getDataLayout());
  auto size_type_str = size_type.get_code().value_or("u64");

  std::string formula =
      // The key type of a bitmap is the size type.
      "keytype(C, " + size_type_str + ") :- use_proxy(C), collection(C).\n" +

      // Select a bitmap implementation.
      "select(C, bitmap) :- use_proxy(C), collection(C), valtype(C, T), T != void.\n"
      "select(C, bitset) :- use_proxy(C), collection(C), valtype(C, void).\n"
      "impl(bitmap).\n"

      // The bitmap _can_ be a sequence implementation.
      // +"{seq(C)} :- use_bitmap(C), collection(C).";
      //
      ;

  return "impl(bitmap). impl(bitset). impl(boost_dynamic_bitset).";
}

std::pair<std::string, std::string> ProxyOpportunity::formulate(
    FormulaEnvironment &env) {

  // Unpack the opportunity.
  auto &proxy = this->proxy;

  // Get the id for the proxy.
  auto proxy_id = std::to_string(ProxyManager::get_id(proxy));

  // Construct the head fact.
  std::string head = "use_proxy(" + proxy_id + ")";

  // Construct the formula.
  std::string formula = "";

  // Formulate the proxy.
  if (auto *natural = dyn_cast<NaturalProxy>(&proxy)) {

    // We can always use a natural proxy, it has no other effects.
    formula += head + ".\n";

  } else if (auto *artificial = dyn_cast<ArtificialProxy>(&proxy)) {

    // We may use an artificial proxy, but it does have a cost.
    formula += "{ " + head + " }.\n";
  } else {
    MEMOIR_UNREACHABLE("Unhandled proxy type.");
  }

  // Fetch the size type code.
  auto size_type = Type::get_size_type(env.module().getDataLayout());
  auto size_type_str = size_type.get_code().value_or("u64");

  // For each allocation, add the type mutation from using the proxy.
  for (auto *alloc : this->allocations) {
    auto alloc_id = std::to_string(env.get_id(alloc->getCallInst()));

    auto collection_head = "collection(" + alloc_id + ")";

    // The key type is converted if we use the proxy.
    // formula += "keytype(" + alloc_id + ", " + size_type_str + ") :- "
    // + collection_head + ", " + head + ".\n";

    // Select a bitmap.
    formula += "select(" + alloc_id + ", bitmap) :- " + collection_head + ", "
               + head + ", valtype(" + alloc_id + ", T), T != void.\n";
    formula += "select(" + alloc_id + ", boost_dynamic_bitset) :-  "
               + collection_head + ", " + head + ", valtype(" + alloc_id
               + ", void).\n";

    // The allocation _can_ become a sequence if we use the proxy.
    // TODO: this is disabled for the time being.
    // formula +=
    //     "{seq(" + alloc_id + ")} :- " + collection_head + ", " + head +
    //     ".\n";
  }

  return std::make_pair(head, formula);
}

namespace detail {

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
      if (isa<AssocInsertInst>(memoir_inst) or isa<AssocRemoveInst>(memoir_inst)
          or isa<AssocWriteInst>(memoir_inst) or isa<RetPHIInst>(memoir_inst)
          or isa<UsePHIInst>(memoir_inst) or isa<ClearInst>(memoir_inst)) {

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

        } else {
          // Gather uses of the closed argument.
          gather_redefinitions(fold->getClosedArgument(use), redefinitions);
        }
      }
    }
  }

  return;
}

void gather_uses_to_proxy(llvm::Value &V,
                          set<llvm::Use *> &to_encode,
                          set<llvm::Use *> &to_decode) {
  // From a given collection, V, gather all uses that need to be either encoded
  // or decoded.
  for (auto &use : V.uses()) {
    auto *user = use.getUser();

    if (auto *read = into<AssocReadInst>(user)) {
      to_encode.insert(&read->getKeyOperandAsUse());

    } else if (auto *write = into<AssocWriteInst>(user)) {
      to_encode.insert(&write->getKeyOperandAsUse());

    } else if (auto *get = into<AssocGetInst>(user)) {
      to_encode.insert(&write->getKeyOperandAsUse());

    } else if (auto *insert = into<AssocInsertInst>(user)) {
      to_encode.insert(&insert->getInsertionPointAsUse());

    } else if (auto *remove = into<AssocRemoveInst>(user)) {
      to_encode.insert(&remove->getKeyAsUse());

    } else if (auto *has = into<AssocHasInst>(user)) {
      to_encode.insert(&has->getKeyOperandAsUse());

    } else if (auto *fold = into<FoldInst>(user)) {
      // If the use is the collection being folded over, add all uses of the
      // index argument to the set of uses to decode.
      if (&fold->getCollectionAsUse() == &use) {
        // Fetch the index argument.
        auto &index_arg = fold->getIndexArgument();

        // Add all uses of the index argument to the set of values to decode.
        for (auto &index_use : index_arg.uses()) {
          to_decode.insert(&index_use);
        }
      }
    }
  }

  return;
}

ordered_map<llvm::Value *, set<Content *>> gather_content_sources(Content &C) {

  ordered_map<llvm::Value *, set<Content *>> sources = {};

  println("Gathering ", C);

  if (auto *elements = dyn_cast<ElementsContent>(&C)) {
    sources[&elements->collection()].insert(&C);
  } else if (auto *keys = dyn_cast<KeysContent>(&C)) {
    sources[&keys->collection()].insert(&C);
  } else if (auto *field = dyn_cast<FieldContent>(&C)) {
    // Wrap the results in the field content.
    auto parent_sources = gather_content_sources(field->parent());

    for (auto [val, contents] : parent_sources) {
      for (auto *content : contents) {
        auto &field_wrapper =
            Content::create<FieldContent>(*content, field->field_index());
        sources[val].insert(&field_wrapper);
      }
    }

  } else if (auto *subset = dyn_cast<SubsetContent>(&C)) {
    return gather_content_sources(subset->content());
  } else if (auto *cond = dyn_cast<ConditionalContent>(&C)) {
    return gather_content_sources(cond->content());
  } else if (auto *tuple = dyn_cast<TupleContent>(&C)) {
    for (auto *nested : tuple->elements()) {
      auto nested_sources = gather_content_sources(*nested);
      for (auto [val, contents] : nested_sources) {
        for (auto *content : contents) {
          sources[val].insert(content);
        }
      }
    }
  } else if (auto *union_content = dyn_cast<UnionContent>(&C)) {
    auto lhs_sources = gather_content_sources(union_content->lhs());
    auto rhs_sources = gather_content_sources(union_content->rhs());
    for (auto &child_sources : { lhs_sources, rhs_sources }) {
      for (auto [val, contents] : child_sources) {
        for (auto *content : contents) {
          sources[val].insert(content);
        }
      }
    }
  }

  return sources;
}

} // namespace detail

bool ProxyOpportunity::exploit(
    std::function<Selection &(llvm::Value &)> get_selection,
    llvm::ModuleAnalysisManager &MAM) {

  bool modified = false;

  // Collect the set of redefinitions of each allocation involved.
  ordered_map<CollectionAllocInst *, set<llvm::Value *>> redefinitions = {};
  for (auto *alloc : this->allocations) {
    detail::gather_redefinitions(alloc->getCallInst(), redefinitions[alloc]);
  }
  println("Collected set of redefinitions.");

  // Collect the set of uses that need to be updated to use the proxy space.
  // We will separate these into uses that need the be encoded/decoded.
  set<llvm::Use *> to_encode = {};
  set<llvm::Use *> to_decode = {};
  for (auto *alloc : this->allocations) {
    // Fetch the selection, its type may impact the uses to consider.
    auto &selection = get_selection(alloc->getCallInst());
    auto &type = selection.type();

    // Iterate over all uses of redefinitions to find any uses that need
    // updated.
    for (auto *redef : redefinitions[alloc]) {
      detail::gather_uses_to_proxy(*redef, to_encode, to_decode);
    }
  }
  println("  before trimming:");
  println("  ", to_encode.size(), " uses to encode");
  println("  ", to_decode.size(), " uses to decode");

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
  for (auto *encodee : to_encode) {
    auto found = to_decode.find(encodee);
    if (found != to_decode.end()) {
      auto *decodee = *found;
      trim_to_encode.insert(encodee);
      trim_to_decode.insert(decodee);
    }
  }

  // Erase the uses that we identified to trim.
  for (auto *use_to_trim : trim_to_decode) {
    to_decode.erase(use_to_trim);
  }
  for (auto *use_to_trim : trim_to_encode) {
    to_encode.erase(use_to_trim);
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

  // If the proxy is artificial, construct it.
  if (auto *artificial_proxy = dyn_cast<ArtificialProxy>(&this->proxy)) {
    // We need to find _where_ we will construct the proxy.
    // There are two options (so far):
    //   1. Find a program point that is dominated by all of the contents, and
    //      dominates each of the uses of the original contents
    //      (there may be multiple).
    //   2. Construct the proxy while you construct the original
    //      contents.
    // With this choice we are balancing the volume of store instructions
    // introduced at once, if we fill up either the cache or LDST queue we will
    // significantly slow down the construction.

    // This implementation is currently only for case 1.

    // Fetch the contents that need to be proxied.
    auto &contents = artificial_proxy->proxied();

    // Find all definitions that used within the contents.
    auto sources = detail::gather_content_sources(contents);

    println("Found ", sources.size(), " sources in the proxied contents.");

    // Find a program point that is dominated by all of the sources.
    llvm::Instruction *construction_point = nullptr;
    for (auto [src, _contents] : sources) {

      // DEBUG printing.
      print("Current construction point:");
      if (construction_point) {
        println(*construction_point);
      } else {
        println("NULL");
      }
      println("Merging with ", *src);

      // Fetch the current construction point's parent function, if it exists.
      auto *function =
          construction_point ? construction_point->getFunction() : nullptr;

      if (auto *src_inst = dyn_cast<llvm::Instruction>(src)) {

        // If this is the first source, set the point.
        if (not construction_point) {
          construction_point = src_inst;
          continue;
        }

        // Fetch the parent function.
        auto &src_function =
            MEMOIR_SANITIZE(src_inst->getFunction(),
                            "Source instruction has no parent function!");

        // If this source and the current construction point are in the same
        // function, find their common dominator.
        if (function == &src_function) {
          // Fetch the dominator tree.
          auto &M = MEMOIR_SANITIZE(src_function.getParent(),
                                    "Function has no parent module!");
          auto &FAM = GET_FUNCTION_ANALYSIS_MANAGER(MAM, M);
          auto *DT =
              FAM.getCachedResult<llvm::DominatorTreeAnalysis>(src_function);

          // Set the construction point to the nearest common dominator.
          construction_point =
              DT->findNearestCommonDominator(construction_point, src_inst);
          continue;

        } else {
          MEMOIR_UNREACHABLE(
              "Sources are defined in different function, currently unhandled.");
        }

        // TODO

      } else if (auto *src_arg = dyn_cast<llvm::Argument>(src)) {

        // Fetch the parent function.
        auto &src_function =
            MEMOIR_SANITIZE(src_arg->getParent(),
                            "Source argument has no parent function!");

        // If this is the first source, set the point.
        if (not construction_point) {
          auto &function_entry = src_function.getEntryBlock();
          auto *first_inst = function_entry.getFirstNonPHI();

          construction_point = first_inst;
          continue;

        } else {
          MEMOIR_UNREACHABLE(
              "Multiple sources where one is an argument is currently unhandled.");
        }

        // TODO
      }
    }

    // Check that we actually found a program point.
    if (not construction_point) {
      MEMOIR_UNREACHABLE(
          "Failed to find a construction point for artificial proxy.");
    }

    // If the construction point is a PHI, find a safe spot to construct.
    if (isa<llvm::PHINode>(construction_point)) {
      bool resolved = false;

      // If the PHI parent has a single incoming edge, and the incoming edge is
      // the only outgoing edge of the predecessor, set the construction point
      // to it.
      auto &construction_block =
          MEMOIR_SANITIZE(construction_point->getParent(),
                          "Construction point PHI has no parent basic block.");
      if (auto *single_pred = construction_block.getSinglePredecessor()) {
        if (single_pred->getSingleSuccessor() == &construction_block) {
          construction_point = single_pred->getTerminator();
          resolved = true;
        }
      }

      // Otherwise, set the construction point to the first non-PHI.
      if (not resolved) {
        construction_point = construction_block.getFirstNonPHI();
      }
    }

    println("Found point to construct proxy:");
    println(*construction_point);

    // NOTE: If we wanted to construct the artificial proxy on the fly, the
    // following logic would need to change.

    // Iterate over the call graph, and find where we need to pass the proxy.
    set<llvm::Function *> functions_with_uses = {};
    for (auto uses : { to_encode, to_decode }) {
      for (auto *use : uses) {
        auto *user_as_inst = dyn_cast<llvm::Instruction>(use->getUser());
        if (not user_as_inst) {
          continue;
        }

        auto *user_function = user_as_inst->getFunction();
        functions_with_uses.insert(user_function);
      }
    }

    functions_with_uses.erase(construction_point->getFunction());

    // Add a new argument to the function for the proxy.
    if (not functions_with_uses.empty()) {
      println("  Proxy is used in ");
      for (auto *func : functions_with_uses) {
        println("    ", func->getName());
      }
    }

    // Fetch LLVM context.
    auto &context = construction_point->getContext();
    auto &module = MEMOIR_SANITIZE(construction_point->getModule(),
                                   "Construction point has no module.");
    auto &data_layout = module.getDataLayout();

    // Allocate the proxy.
    MemOIRBuilder builder(construction_point);

    auto &content_type =
        MEMOIR_SANITIZE(contents.type(), "Could not determine content type.");

    auto &size_type = Type::get_size_type(data_layout);
    auto &llvm_size_type = *size_type.get_llvm_type(context);
    auto size_type_bitwidth = size_type.getBitWidth();

    // Determine which proxies we need.
    bool build_encoder = to_encode.size() > 0;
    bool build_decoder = to_decode.size() > 0;

    llvm::Value *decoder = nullptr;
    CollectionType *decoder_type = nullptr;
    if (build_decoder) {
      auto *decoder_alloc = builder.CreateSequenceAllocInst(content_type,
                                                            size_t(0),
                                                            "proxy.decode.");
      decoder = &decoder_alloc->getCallInst();
      decoder_type = &decoder_alloc->getCollectionType();
    }

    llvm::Value *encoder = nullptr;
    CollectionType *encoder_type = nullptr;
    if (build_encoder) {
      auto *encoder_alloc = builder.CreateAssocArrayAllocInst(content_type,
                                                              size_type,
                                                              "proxy.encode.");
      encoder = &encoder_alloc->getCallInst();
      encoder_type = &encoder_alloc->getCollectionType();
    }

    // After the definition of each source, insert
    // its values into the proxy.
    for (auto [src, src_contents] : sources) {
      // Determine the insertion point.
      llvm::Instruction *insertion_point;
      if (auto *src_inst = dyn_cast<llvm::Instruction>(src)) {
        insertion_point = src_inst->getNextNode();
      } else if (auto *src_arg = dyn_cast<llvm::Argument>(src)) {
        auto *src_parent = src_arg->getParent();
        insertion_point = src_parent->getEntryBlock().getFirstNonPHI();
      } else {
        MEMOIR_UNREACHABLE("Unhandle source type");
      }

      // If the construction and insertion points are in the same basic block,
      // make sure that insertion happens after construction.
      if (insertion_point->getParent() == construction_point->getParent()) {
        insertion_point = construction_point;
      }

      // Move the builder's insertion point to be just after the source
      // definition.
      builder.SetInsertPoint(insertion_point);

      // Get the type of the source.
      auto &src_type =
          MEMOIR_SANITIZE(type_of(*src), "Could not get type of source.");
      llvm::Type *key_type, *val_type;
      if (auto *src_seq_type = dyn_cast<SequenceType>(&src_type)) {
        key_type = &llvm_size_type;
        val_type = src_seq_type->getElementType().get_llvm_type(context);

      } else if (auto *src_assoc_type = dyn_cast<AssocType>(&src_type)) {
        key_type = src_assoc_type->getKeyType().get_llvm_type(context);
        val_type = src_assoc_type->getElementType().get_llvm_type(context);

      } else {
        MEMOIR_UNREACHABLE("Unhandled source type.");
      }

      // Create the function type for the fold body.
      auto *ptr_type = builder.getPtrTy(0);
      vector<llvm::Type *> arg_types = { &llvm_size_type, key_type, val_type };
      if (encoder) {
        arg_types.push_back(ptr_type);
      }
      if (decoder) {
        arg_types.push_back(ptr_type);
      }
      auto *outer_fold_function_type =
          llvm::FunctionType::get(&llvm_size_type,
                                  llvm::ArrayRef<llvm::Type *>(arg_types),
                                  /* isVarArg = */ false);

      // Create the fold body function.
      auto *outer_fold_body = llvm::Function::Create(
          outer_fold_function_type,
          llvm::GlobalValue::LinkageTypes::InternalLinkage,
          "proxy.init.outer",
          module);

      // Create the fold instruction.
      vector<llvm::Value *> closed = {};
      if (encoder) {
        closed.push_back(encoder);
      }
      if (decoder) {
        closed.push_back(decoder);
      }

      llvm::Value *initial_accum = nullptr;
      if (encoder) {
        initial_accum = &builder.CreateSizeInst(encoder)->getCallInst();
      } else if (decoder) {
        initial_accum = &builder.CreateSizeInst(decoder)->getCallInst();
      } else {
        MEMOIR_UNREACHABLE(
            "Trying to initialize encoder/decoder that we don't have.");
      }
      auto *outer_fold_inst = builder.CreateFoldInst(size_type,
                                                     initial_accum,
                                                     src,
                                                     outer_fold_body,
                                                     closed);

      if (decoder) {
        auto *decoder_ret_phi =
            builder.CreateRetPHI(decoder, &outer_fold_inst->getLLVMFunction());
        decoder = &decoder_ret_phi->getCallInst();
      }

      if (encoder) {
        auto *encoder_ret_phi =
            builder.CreateRetPHI(encoder, &outer_fold_inst->getLLVMFunction());
        encoder = &encoder_ret_phi->getCallInst();
      }

      // Populate the fold body.

      // - Create the entry block.
      auto *outer_fold_entry =
          llvm::BasicBlock::Create(context, "", outer_fold_body);

      // - Unpack the fold body's arguments.
      auto *accum_arg = outer_fold_body->getArg(0);
      auto *outer_key_arg = outer_fold_body->getArg(1);
      auto *outer_val_arg = outer_fold_body->getArg(2);
      auto *encoder_arg = encoder ? outer_fold_body->getArg(3) : nullptr;
      auto *decoder_arg = decoder ? (encoder ? outer_fold_body->getArg(4)
                                             : outer_fold_body->getArg(3))
                                  : nullptr;

      // We will use the constant -1, to say that the value has not been found.
      auto *not_found_constant = builder.getIntN(size_type_bitwidth, -1);

      // Set the builder's insertion point to the current outer block.
      builder.SetInsertPoint(outer_fold_entry);

      // Annotate the collection arguments with their type.
      if (encoder_arg) {
        builder.CreateAssertTypeInst(encoder_arg, *encoder_type);
      }

      if (decoder_arg) {
        builder.CreateAssertTypeInst(decoder_arg, *decoder_type);
      }

      if (auto *src_seq_type = dyn_cast<SequenceType>(&src_type)) {
        auto &elem_type = src_seq_type->getElementType();
        if (isa<StructType>(&elem_type)) {
          builder.CreateAssertTypeInst(outer_val_arg, elem_type);
        }
      } else if (auto *src_assoc_type = dyn_cast<AssocArrayType>(&src_type)) {
        auto &key_type = src_assoc_type->getKeyType();
        if (isa<StructType>(&key_type)) {
          builder.CreateAssertTypeInst(outer_key_arg, key_type);
        }

        auto &val_type = src_assoc_type->getValueType();
        if (isa<StructType>(&val_type)) {
          builder.CreateAssertTypeInst(outer_val_arg, val_type);
        }
      }

      // For each content being proxied from this source, insert the value.
      llvm::Value *current_accum = accum_arg;
      llvm::Value *current_encoder = encoder_arg;
      llvm::Value *current_decoder = decoder_arg;
      llvm::BasicBlock *current_outer_block = outer_fold_entry;
      for (auto *src_content : src_contents) {

        println("  Initializing proxy with content: ", *src_content);

        // Get the value to search for.
        // TODO: Make this more robust.
        llvm::Value *searchee = nullptr;
        if (auto *field = dyn_cast<FieldContent>(src_content)) {

          auto &parent_content = field->parent();

          llvm::Value *searchee_parent = nullptr;
          if (auto *keys = dyn_cast<KeysContent>(&parent_content)) {
            searchee_parent = outer_key_arg;
          } else if (auto *elems = dyn_cast<ElementsContent>(&parent_content)) {
            searchee_parent = outer_val_arg;
          }

          MEMOIR_NULL_CHECK(searchee_parent,
                            "Unhandle field content being proxied.");

          // Construct the field access.
          auto &element_type = encoder ? encoder_type->getElementType()
                                       : decoder_type->getElementType();
          auto *field_read = builder.CreateStructReadInst(element_type,
                                                          searchee_parent,
                                                          field->field_index());
          searchee = &field_read->getCallInst();

        } else if (auto *keys = dyn_cast<KeysContent>(src_content)) {
          searchee = outer_key_arg;
        } else if (auto *elems = dyn_cast<ElementsContent>(src_content)) {
          searchee = outer_val_arg;
        }

        // Construct a conditional insert.
        //   if (not has(encoder, searchee)):
        //     id = size(encoder)
        //     encoder' = write(id, insert(encoder, searchee), searchee)
        //   encoder" = PHI encoder', encoder

        // Set the builder's insertion point to the current outer block.
        // builder.SetInsertPoint(current_outer_block);

        // Construct the comparison.
        auto *check_present =
            &builder.CreateAssocHasInst(current_encoder, searchee)
                 ->getCallInst();

        // Construct the if block.
        auto *if_block = llvm::BasicBlock::Create(context, "", outer_fold_body);

        // Construct the continuation block.
        auto *continue_block =
            llvm::BasicBlock::Create(context, "", outer_fold_body);

        builder.CreateCondBr(check_present, continue_block, if_block);

        // Populate the if block.
        builder.SetInsertPoint(if_block);

        llvm::Value *new_encoder = nullptr;
        if (current_encoder) {
          auto *encoder_insert =
              builder.CreateAssocInsertInst(current_encoder, searchee);
          auto *encoder_write =
              builder.CreateAssocWriteInst(size_type,
                                           current_accum,
                                           &encoder_insert->getCallInst(),
                                           searchee);
          new_encoder = &encoder_write->getCallInst();
        }

        llvm::Value *new_decoder = nullptr;
        if (current_decoder) {
          auto *decoder_end = builder.CreateEndInst();
          auto *decoder_insert =
              builder.CreateSeqInsertValueInst(size_type,
                                               searchee,
                                               current_decoder,
                                               &decoder_end->getCallInst());
          new_decoder = &decoder_insert->getCallInst();
        }

        auto *accum_plus_one =
            builder.CreateAdd(current_accum,
                              builder.getIntN(size_type_bitwidth, 1));

        builder.CreateBr(continue_block);

        // Populate the continuation block.
        builder.SetInsertPoint(continue_block);

        auto *accum_phi = builder.CreatePHI(current_accum->getType(), 2);
        accum_phi->addIncoming(current_accum, current_outer_block);
        accum_phi->addIncoming(accum_plus_one, if_block);
        current_accum = accum_phi;

        if (current_encoder) {
          auto *encoder_phi = builder.CreatePHI(ptr_type, 2);
          encoder_phi->addIncoming(current_encoder, current_outer_block);
          encoder_phi->addIncoming(new_encoder, if_block);
          current_encoder = encoder_phi;
        }

        if (current_decoder) {
          auto *decoder_phi = builder.CreatePHI(ptr_type, 2);
          decoder_phi->addIncoming(current_decoder, current_outer_block);
          decoder_phi->addIncoming(new_decoder, if_block);
          current_decoder = decoder_phi;
        }

        // Update the current state.
        current_outer_block = continue_block;

        MEMOIR_NULL_CHECK(searchee, "Unhandle content being proxied.");
      }

      // - Mark the current values as the live-outs.
      if (current_encoder) {
        if (auto *inst = dyn_cast<llvm::Instruction>(current_encoder)) {
          auto live_out_metadata = Metadata::get_or_add<LiveOutMetadata>(*inst);
          live_out_metadata.setArgNo(encoder_arg->getArgNo());
        }
      }

      if (current_decoder) {
        if (auto *inst = dyn_cast<llvm::Instruction>(current_decoder)) {
          auto live_out_metadata = Metadata::get_or_add<LiveOutMetadata>(*inst);
          live_out_metadata.setArgNo(decoder_arg->getArgNo());
        }
      }

      // - Add the return instruction.
      builder.CreateRet(current_accum);
    }

    // Make the proxy available at all uses.

    // Find the set of functions that need the proxy.
    set<llvm::Function *> to_encode_functions = {};
    for (auto *use : to_encode) {
      auto *user = use->getUser();
      auto *user_as_inst = dyn_cast<llvm::Instruction>(user);
      if (not user_as_inst) {
        continue;
      }
      auto *user_bb = user_as_inst->getParent();
      auto *user_func = user_bb->getParent();

      to_encode_functions.insert(user_func);
    }
    to_encode_functions.erase(construction_point->getFunction());

    set<llvm::Function *> to_decode_functions = {};
    for (auto *use : to_decode) {
      auto *user = use->getUser();
      auto *user_as_inst = dyn_cast<llvm::Instruction>(user);
      if (not user_as_inst) {
        continue;
      }

      auto *user_bb = user_as_inst->getParent();
      auto *user_func = user_bb->getParent();

      to_decode_functions.insert(user_func);
    }
    to_decode_functions.erase(construction_point->getFunction());

    // Determine the set of functions that we need to pass the proxy to.
    set<llvm::CallBase *> calls_to_patch_with_encoder = {};
    set<FoldInst *> folds_to_patch_with_encoder = {};
    for (auto *to_encode_func : to_encode_functions) {
      if (auto *single_use = to_encode_func->getSingleUndroppableUse()) {
        auto *single_user = single_use->getUser();
        if (auto *fold = into<FoldInst>(single_user)) {
          folds_to_patch_with_encoder.insert(fold);
        }
      } else {
        for (auto &use : to_encode_func->uses()) {
          if (auto *call = dyn_cast<llvm::CallBase>(use.getUser())) {
            calls_to_patch_with_encoder.insert(call);
          }
        }
      }
    }

    // Determine the set of functions that we need to pass the proxy to.
    set<llvm::CallBase *> calls_to_patch_with_decoder = {};
    set<FoldInst *> folds_to_patch_with_decoder = {};
    for (auto *to_decode_func : to_decode_functions) {
      if (auto *single_use = to_decode_func->getSingleUndroppableUse()) {
        auto *single_user = single_use->getUser();
        if (auto *fold = into<FoldInst>(single_user)) {
          folds_to_patch_with_decoder.insert(fold);
        }
      } else {
        for (auto &use : to_decode_func->uses()) {
          if (auto *call = dyn_cast<llvm::CallBase>(use.getUser())) {
            calls_to_patch_with_decoder.insert(call);
          }
        }
      }
    }

    // For each call/fold, we will create a temporary argument (a load/store to
    // a global variable) that will be destructed later.

    // First, construct the global variable.
    auto *ptr_type = builder.getPtrTy(0);

    // Then, construct the load/store for each call site.
    map<llvm::Function *, llvm::Instruction *> function_to_encoder = {};
    for (auto *fold : folds_to_patch_with_encoder) {
      // Create the store ahead of the fold.
      builder.SetInsertPoint(&fold->getCallInst());

      auto *encoder_global = new llvm::GlobalVariable(
          module,
          ptr_type,
          /* isConstant? */ false,
          llvm::GlobalValue::LinkageTypes::InternalLinkage,
          llvm::Constant::getNullValue(ptr_type),
          "temparg.encoder.");

      auto *store = builder.CreateStore(encoder, encoder_global);
      Metadata::get_or_add<TempArgumentMetadata>(*store);

      // Create the load in the entry of the fold body.
      auto &body = fold->getFunction();
      builder.SetInsertPoint(body.getEntryBlock().getFirstNonPHI());

      auto *load = builder.CreateLoad(ptr_type, encoder_global);
      function_to_encoder[&body] = load;
      Metadata::get_or_add<TempArgumentMetadata>(*load);
      builder.CreateAssertTypeInst(load, *encoder_type);
    }
    map<llvm::Function *, llvm::Instruction *> function_to_decoder = {};
    for (auto *fold : folds_to_patch_with_decoder) {
      // Create the store ahead of the fold.
      builder.SetInsertPoint(&fold->getCallInst());

      auto *decoder_global = new llvm::GlobalVariable(
          module,
          ptr_type,
          /* isConstant? */ false,
          llvm::GlobalValue::LinkageTypes::InternalLinkage,
          llvm::Constant::getNullValue(ptr_type),
          "temparg.decoder.");

      auto *store = builder.CreateStore(decoder, decoder_global);
      Metadata::get_or_add<TempArgumentMetadata>(*store);

      // Create the load in the entry of the fold body.
      auto &body = fold->getFunction();
      builder.SetInsertPoint(body.getEntryBlock().getFirstNonPHI());

      auto *load = builder.CreateLoad(ptr_type, decoder_global);
      function_to_decoder[&body] = load;
      Metadata::get_or_add<TempArgumentMetadata>(*load);
      builder.CreateAssertTypeInst(load, *decoder_type);
    }

    function_to_encoder[construction_point->getFunction()] =
        cast<llvm::Instruction>(encoder);

    // TODO: do the same for calls.
    for (auto *call : calls_to_patch_with_encoder) {
    }

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

      // Then, read the value from the encoder.
      auto *encoded =
          &builder.CreateAssocReadInst(size_type, encoder, used)->getCallInst();

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
      auto *decoded = &builder.CreateIndexReadInst(content_type, decoder, used)
                           ->getCallInst();

      use->set(decoded);
    }
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

  return modified;
}
// ===========================

} // namespace folio
