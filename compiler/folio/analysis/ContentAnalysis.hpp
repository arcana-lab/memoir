#include "llvm/Pass.h"

#include "llvm/IR/PassManager.h"

#include "memoir/ir/InstVisitor.hpp"

#include "memoir/support/InternalDatatypes.hpp"

#include "folio/analysis/ContentSummary.hpp"

namespace folio {

using Contents = typename llvm::memoir::map<llvm::Value *, Content *>;

struct ContentAnalysisDriver
  : llvm::memoir::InstVisitor<ContentAnalysisDriver, Content &> {
  friend class llvm::memoir::InstVisitor<ContentAnalysisDriver, Content &>;
  friend class llvm::InstVisitor<ContentAnalysisDriver, Content &>;

public:
  // Constructor.
  ContentAnalysisDriver(Contents &result, llvm::Module &M);

protected:
  // Driver methods.
  void initialize();
  void simplify();

  // Helper methods.
  void summarize(llvm::Value &value, Content &content);
  void summarize(llvm::memoir::MemOIRInst &value, Content &content);

  // Visitor methods.
  Content &analyze(llvm::Value &V);
  Content &analyze(llvm::memoir::MemOIRInst &I);
  Content &visitArgument(llvm::Argument &I);
  Content &visitInstruction(llvm::Instruction &I);
  Content &visitSequenceAllocInst(llvm::memoir::SequenceAllocInst &I);
  Content &visitAssocArrayAllocInst(llvm::memoir::AssocArrayAllocInst &I);
  Content &visitSeqInsertInst(llvm::memoir::SeqInsertInst &I);
  Content &visitSeqInsertValueInst(llvm::memoir::SeqInsertValueInst &I);
  Content &visitIndexWriteInst(llvm::memoir::IndexWriteInst &I);
  Content &visitAssocInsertInst(llvm::memoir::AssocInsertInst &I);
  Content &visitAssocWriteInst(llvm::memoir::AssocWriteInst &I);
  Content &visitStructReadInst(llvm::memoir::StructReadInst &I);
  Content &visitFoldInst(llvm::memoir::FoldInst &I);
  Content &visitRetPHIInst(llvm::memoir::RetPHIInst &I);
  Content &visitUsePHIInst(llvm::memoir::UsePHIInst &I);
  Content &visitPHINode(llvm::PHINode &I);

  // Owned state.
  llvm::memoir::set<llvm::Value *> visited;

  // Borrowed state.
  Contents &result;
  llvm::Module &M;
};

class ContentAnalysis : public llvm::AnalysisInfoMixin<ContentAnalysis> {
  friend struct llvm::AnalysisInfoMixin<ContentAnalysis>;
  static llvm::AnalysisKey Key;

public:
  using Result = typename folio::Contents;
  Result run(llvm::Module &M, llvm::ModuleAnalysisManager &MAM);
};

} // namespace folio
