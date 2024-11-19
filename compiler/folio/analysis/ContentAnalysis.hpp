#include "llvm/Pass.h"

#include "llvm/IR/PassManager.h"

#include "llvm/Analysis/LoopInfo.h"

#include "memoir/ir/InstVisitor.hpp"

#include "memoir/support/InternalDatatypes.hpp"

#include "folio/analysis/Content.hpp"

namespace folio {

struct ContentAnalysisDriver
  : llvm::memoir::InstVisitor<ContentAnalysisDriver, ContentSummary> {
  friend class llvm::memoir::InstVisitor<ContentAnalysisDriver, ContentSummary>;
  friend class llvm::InstVisitor<ContentAnalysisDriver, ContentSummary>;

public:
  // Constructor.
  ContentAnalysisDriver(Contents &result,
                        llvm::Module &M,
                        std::function<llvm::Loop *(llvm::Instruction &)>);

protected:
  // Driver methods.
  void initialize();
  void simplify();

  // Helper methods.
  void summarize(llvm::Value &value, ContentSummary content);
  void summarize(llvm::memoir::MemOIRInst &value, ContentSummary content);

  bool is_in_scope(llvm::Value &V);

  std::pair<llvm::memoir::set<llvm::Value *>, bool> set_recurse();
  void restore_recurse(std::pair<llvm::memoir::set<llvm::Value *>, bool> old);

  // Visitor methods.
  ContentSummary analyze(llvm::Value &V, bool force_recurse = false);
  ContentSummary analyze(llvm::memoir::MemOIRInst &I,
                         bool force_recurse = false);

  ContentSummary visitArgument(llvm::Argument &I);
  ContentSummary visitInstruction(llvm::Instruction &I);

  ContentSummary visitSequenceAllocInst(llvm::memoir::SequenceAllocInst &I);
  ContentSummary visitAssocArrayAllocInst(llvm::memoir::AssocArrayAllocInst &I);

  ContentSummary visitSeqInsertInst(llvm::memoir::SeqInsertInst &I);
  ContentSummary visitSeqInsertValueInst(llvm::memoir::SeqInsertValueInst &I);
  ContentSummary visitIndexWriteInst(llvm::memoir::IndexWriteInst &I);

  ContentSummary visitAssocInsertInst(llvm::memoir::AssocInsertInst &I);
  ContentSummary visitAssocWriteInst(llvm::memoir::AssocWriteInst &I);

  ContentSummary visitIndexReadInst(llvm::memoir::IndexReadInst &I);
  ContentSummary visitAssocReadInst(llvm::memoir::AssocReadInst &I);
  ContentSummary visitStructReadInst(llvm::memoir::StructReadInst &I);

  ContentSummary visitAssocKeysInst(llvm::memoir::AssocKeysInst &I);

  ContentSummary contextualize_fold(llvm::memoir::FoldInst &I,
                                    ContentSummary summary);
  ContentSummary visitFoldInst(llvm::memoir::FoldInst &I);

  ContentSummary visitClearInst(llvm::memoir::ClearInst &I);

  ContentSummary contextualize_call(llvm::CallBase &I,
                                    llvm::Function &F,
                                    ContentSummary summary);
  ContentSummary visitRetPHIInst(llvm::memoir::RetPHIInst &I);
  ContentSummary visitUsePHIInst(llvm::memoir::UsePHIInst &I);

  ContentSummary visitPHINode(llvm::PHINode &I);

  // Owned state.
  llvm::memoir::set<llvm::Value *> visited;
  llvm::memoir::map<llvm::Value *, ContentSummary> temporaries;
  bool recurse;
  std::function<llvm::Loop *(llvm::Instruction &)> get_loop_for;

  // Borrowed state.
  Contents &result;
  llvm::Module &M;
  llvm::Instruction *current;
};

class ContentAnalysis : public llvm::AnalysisInfoMixin<ContentAnalysis> {
  friend struct llvm::AnalysisInfoMixin<ContentAnalysis>;
  static llvm::AnalysisKey Key;

public:
  using Result = typename folio::Contents;
  Result run(llvm::Module &M, llvm::ModuleAnalysisManager &MAM);
};

} // namespace folio
