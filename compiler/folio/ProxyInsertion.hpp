#ifndef FOLIO_TRANSFORMS_PROXYINSERTION_H
#define FOLIO_TRANSFORMS_PROXYINSERTION_H

#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "memoir/analysis/BoundsCheckAnalysis.hpp"
#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/Types.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/DataTypes.hpp"

#include "folio/Candidate.hpp"
#include "folio/ObjectInfo.hpp"
#include "folio/Utilities.hpp"

namespace folio {

using llvm::memoir::AllocInst;
using llvm::memoir::Type;
using Builder = typename llvm::memoir::MemOIRBuilder;

struct ProxyInsertion {
public:
  // Helper types.
  using GetDominatorTree =
      std::function<llvm::DominatorTree &(llvm::Function &)>;
  using GetBoundsChecks =
      std::function<llvm::memoir::BoundsCheckResult &(llvm::Function &)>;
  using Enumerated = Map<NestedObject, SmallSet<Candidate *, 1>>;

  struct TransformInfo {
    Map<llvm::Function *, Set<llvm::Value *>> encoded;
    Map<llvm::Function *, Set<llvm::Use *>> to_decode, to_encode, to_addkey;
    Type *key_type = NULL, *encoder_type = NULL, *decoder_type = NULL;
    bool build_encoder, build_decoder;
    llvm::FunctionCallee addkey_callee;
    llvm::GlobalVariable *enc_global, *dec_global;
  };

  // Constructors.
  ProxyInsertion(llvm::Module &module,
                 GetDominatorTree get_dominator_tree,
                 GetBoundsChecks get_bounds_checks);

  // Driver functions.
  void analyze();

  void optimize();

  void prepare();

  bool transform();

  // Helper functions.
  static Option<std::string> get_enumerated_impl(Type &type,
                                                 bool is_nested = false);

protected:
  void gather_assoc_objects();
  void gather_assoc_objects(AllocInst &alloc);
  void gather_assoc_objects(AllocInst &alloc, Type &type, Offsets offsets = {});

  void gather_propagators();
  void gather_propagators(const Set<Type *> &types, AllocInst &alloc);
  void gather_propagators(const Set<Type *> &types,
                          AllocInst &alloc,
                          Type &type,
                          OffsetsRef offsets = {});

  void gather_abstract_objects();
  void gather_abstract_objects(ObjectInfo &object);

  void flesh_out(Candidate &arguments);

  void share_proxies();

  void unify_bases();

  ObjectInfo *find_base_object(llvm::Value &V,
                               llvm::memoir::AccessInst &access);

  llvm::Module &module;
  Vector<BaseObjectInfo> objects, propagators;
  Vector<ArgObjectInfo> arguments;
  Vector<Candidate> candidates;
  Enumerated enumerated;

  // Group objects by abstract/base candidate
  struct PrioritizeBase {
    bool operator()(ObjectInfo *lhs,
                    ObjectInfo *rhs,
                    size_t lsize,
                    size_t rsize) {
      if (isa<ArgObjectInfo>(lhs))
        return true;
      else
        return lsize < rsize;
    }
  };
  UnionFind<ObjectInfo *, PrioritizeBase> unified;
  Map<ObjectInfo *, SmallVector<ObjectInfo *>> equiv;

  // Transformation.
  Map<ObjectInfo *, TransformInfo> to_transform;

  llvm::Value *load_decoder(Builder &builder, const TransformInfo &info);
  llvm::Value *load_encoder(Builder &builder, const TransformInfo &info);

  void store_decoder(Builder &builder,
                     const TransformInfo &info,
                     llvm::Value *dec);
  void store_encoder(Builder &builder,
                     const TransformInfo &info,
                     llvm::Value *enc);

  /** Check if the enumeration has the given value */
  llvm::Instruction &has_value(Builder &builder,
                               const TransformInfo &info,
                               llvm::Value &value);
  /** Decode the given value */
  llvm::Value &decode_value(Builder &builder,
                            const TransformInfo &info,
                            llvm::Value &value);
  /** Encode the given value */
  llvm::Value &encode_value(Builder &builder,
                            const TransformInfo &info,
                            llvm::Value &value);

  llvm::Value &encode_use(Builder &builder,
                          const TransformInfo &info,
                          llvm::Use &use);

  /** Add the given value to the enumeration */
  llvm::Value &add_value(Builder &builder,
                         const TransformInfo &info,
                         llvm::Value &value);

  void decode_uses();

  void patch_uses();

  ObjectInfo *find_recursive_base(BaseObjectInfo &base);

  void allocate_mappings(BaseObjectInfo &base);

  void promote();

  void mutate_types();

  GetDominatorTree get_dominator_tree;
  GetBoundsChecks get_bounds_checks;
};

} // namespace folio

#endif // FOLIO_TRANSFORMS_PROXYINSERTION_H
