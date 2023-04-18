/*
 * Pass to perform constant propagation on MemOIR collections.
 *
 * Author: Nick Wanninger <ncw@u.northwestern.edu>
 * Created: April 17, 2023
 */

#include "ConstantPropagation.hpp"
#include "memoir/analysis/CollectionAnalysis.hpp"
#include "memoir/analysis/ValueNumbering.hpp"
#include "memoir/ir/InstVisitor.hpp"
#include "memoir/ir/Instructions.hpp"
#include "memoir/support/Print.hpp"
#include "llvm/IR/Value.h"

using namespace llvm::memoir;

namespace constprop {

//===---------------------------------------------------------------------===//
// Constant Propagation Location
//===---------------------------------------------------------------------===//

Location::Location(memoir::Collection &c,
                   SmallVector<memoir::ValueExpression *, 8> &&inds)
  : m_collection(c),
    m_inds(std::move(inds)) {}

memoir::Collection &Location::getCollection(void) const {
  return m_collection;
}

unsigned Location::getNumInds(void) const {
  return m_inds.size();
}

memoir::ValueExpression *Location::getInd(unsigned i) const {
  return m_inds[i];
}

//===---------------------------------------------------------------------===//
// Constant Propagation Analysis and Transformation
//===---------------------------------------------------------------------===//

class ConstPropVisitor
  : public llvm::memoir::InstVisitor<ConstPropVisitor, void> {
  friend class llvm::memoir::InstVisitor<ConstPropVisitor, void>;
  friend class llvm::InstVisitor<ConstPropVisitor, void>;

  memoir::ValueNumbering &VN;

public:
  ConstPropVisitor(memoir::ValueNumbering &VN) : VN(VN) {}

  void visitInstruction(llvm::Instruction &I) {
    // println("Memoir Inst: ", I);
  }
  void visitMemOIRInst(MemOIRInst &I) {
    println("\e[31mConstProp: Unhandled MemOIR Instruction\e[0m ", I);
  }

  void visitIndexReadInst(IndexReadInst &I) {
    auto c = I.getCallInst().getArgOperand(0);
    auto ind = I.getCallInst().getArgOperand(1);
    println(I);
    if (auto cllct = CollectionAnalysis::analyze(*c)) {
      constprop::Location loc(*cllct, { VN.get(*ind) });
      println(loc);
    }
  }

  void visitIndexWriteInst(IndexWriteInst &I) {
    auto c = I.getCallInst().getArgOperand(1);
    auto ind = I.getCallInst().getArgOperand(2);
    println(I);
    if (auto cllct = CollectionAnalysis::analyze(*c)) {
      constprop::Location loc(*cllct, { VN.get(*ind) });
      println(loc);
    }
  }
};

ConstantPropagation::ConstantPropagation(Module &M,
                                         memoir::CollectionAnalysis &CA)
  : M(M),
    CA(CA),
    VN(M) {
  // Nothing.
}

void ConstantPropagation::analyze() {
  // Analyze the program
  println("Constant Prop: Analyze");
  ConstPropVisitor vis(VN);

  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        vis.visit(&I);
      }
    }
  }

  // println("=========================================");

  // for (auto &F : M) {
  //   if (memoir::MetadataManager::hasMetadata(F, MetadataType::INTERNAL)) {
  //     continue;
  //   }

  //   for (auto &BB : F) {
  //     for (auto &I : BB) {
  //       if (auto call_inst = dyn_cast<CallInst>(&I)) {
  //         if (!FunctionNames::is_memoir_call(*call_inst)) {
  //           continue;
  //         }

  //         if (auto cllct = CA.analyze(*call_inst)) {
  //           println("Found collection: ", I);
  //           println(" Collection Type: ", *cllct);
  //         }
  //       }
  //     }
  //   }
  // }
  // println("=========================================");

  return;
}

void ConstantPropagation::transform() {
  // Transform the program
  println("Constant Prop: Transform");

  return;
}

} // namespace constprop
