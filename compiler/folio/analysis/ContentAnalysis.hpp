#include "llvm/Pass.h"

#include "llvm/IR/PassManager.h"

#include "memoir/ir/InstVisitor.hpp"

#include "memoir/support/InternalDatatypes.hpp"

#include "folio/analysis/Content.hpp"

namespace folio {

using ContentSummary = typename std::pair<Content *, Content *>;
using Contents = typename llvm::memoir::map<llvm::Value *, ContentSummary>;

struct ContentAnalysisDriver
  : llvm::memoir::InstVisitor<ContentAnalysisDriver, ContentSummary> {
  friend class llvm::memoir::InstVisitor<ContentAnalysisDriver, ContentSummary>;
  friend class llvm::InstVisitor<ContentAnalysisDriver, ContentSummary>;

public:
  // Constructor.
  ContentAnalysisDriver(Contents &result, llvm::Module &M);

protected:
  // Driver methods.
  void initialize();
  void simplify();

  // Helper methods.
  void summarize(llvm::Value &value, ContentSummary content);
  void summarize(llvm::memoir::MemOIRInst &value, ContentSummary content);

  // Visitor methods.
  ContentSummary analyze(llvm::Value &V);
  ContentSummary analyze(llvm::memoir::MemOIRInst &I);

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

  ContentSummary visitFoldInst(llvm::memoir::FoldInst &I);
  ContentSummary visitRetPHIInst(llvm::memoir::RetPHIInst &I);
  ContentSummary visitUsePHIInst(llvm::memoir::UsePHIInst &I);

  ContentSummary visitPHINode(llvm::PHINode &I);

  // Simplification
  Content *lookup_domain(llvm::Value &V);
  Content *lookup_range(llvm::Value &V);
  Content &simplify(Content &C, llvm::Value *value = nullptr);
  Content &simplifyUnionContent(UnionContent &C, llvm::Value *value);
  Content &simplifyConditionalContent(ConditionalContent &C,
                                      llvm::Value *value);
  Content &simplifyElementsContent(ElementsContent &C, llvm::Value *value);
  Content &simplifyKeysContent(KeysContent &C, llvm::Value *value);

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
