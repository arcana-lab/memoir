#include "memoir/support/Casting.hpp"

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

bool Candidate::build_encoder() const {
  return this->encoded.size() > 0 or this->added.size() > 0;
}

bool Candidate::build_decoder() const {
  return this->decoded.size() > 0;
}

static void print_uses(const Set<llvm::Use *> &to_encode,
                       const Set<llvm::Use *> &to_decode,
                       const Set<llvm::Use *> &to_addkey) {
  println("    ", to_encode.size(), " USES TO ENCODE ");
  for (auto *use : to_encode) {
    infoln(pretty_use(*use));
  }
  infoln();
  println("    ", to_decode.size(), " USES TO DECODE ");
  for (auto *use : to_decode) {
    infoln(pretty_use(*use));
  }
  infoln();
  println("    ", to_addkey.size(), " USES TO ADDKEY ");
  for (auto *use : to_addkey) {
    infoln(pretty_use(*use));
  }
}

static void print_uses(const LocalMap<Set<llvm::Use *>> &to_encode,
                       const LocalMap<Set<llvm::Use *>> &to_decode,
                       const LocalMap<Set<llvm::Use *>> &to_addkey) {
  // TODO
  return;
}

void Candidate::optimize(
    std::function<llvm::DominatorTree &(llvm::Function &)> get_domtree,
    std::function<BoundsCheckResult &(llvm::Function &)> get_bounds_checks) {

  // Collect all of the uses that need to be handled.
  LocalMap<Set<llvm::Use *>> to_decode, to_encode, to_addkey;
  this->gather_uses(this->encoded_values, to_decode, to_encode, to_addkey);

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

  coalesce(decoded, to_decode, get_domtree);
  coalesce(encoded, to_encode, get_domtree);
  coalesce(added, to_addkey, get_domtree);

  // Report the coalescing.
  println("  AFTER COALESCING:");
  println("    USES TO ENCODE ", encoded.size());
  println("    USES TO DECODE ", decoded.size());
  println("    USES TO ADDKEY ", added.size());
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                              const Candidate &candidate) {
  os << "CANDIDATE: ";
  for (const auto *info : candidate) {
    os << "\n  " << *info;
  }
  return os;
}

} // namespace folio
