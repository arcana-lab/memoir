#ifndef MEMOIR_SLICEPROVISIONER_H
#define MEMOIR_SLICEPROVISIONER_H
#pragma once

// LLVM
#include "llvm/IR/Module.h"

// NOELLE
#include "noelle/core/Noelle.hpp"

// MemOIR
#include "memoir/ir/InstVisitor.hpp"
#include "memoir/ir/Instructions.hpp"

#include "memoir/support/InternalDatatypes.hpp"

#include "memoir/analysis/CollectionAnalysis.hpp"
#include "memoir/analysis/SizeAnalysis.hpp"

namespace llvm::memoir {

class SequenceProvisioner : public InstVisitor<SequenceProvisioner> {
  friend class InstVisitor<SequenceProvisioner>;
  friend class llvm::InstVisitor<SequenceProvisioner>;

public:
  SequenceProvisioner(llvm::Module &M,
                      arcana::noelle::Noelle &noelle,
                      CollectionAnalysis &CA,
                      SizeAnalysis &SizeA)
    : M(M),
      noelle(noelle),
      CA(CA),
      SizeA(SizeA) {}

  ~SequenceProvisioner();

  /**
   * Runs the sequence provisioning analysis and transformation on the LLVM
   * Module given to the constructor.
   *
   * returns True if the program was modified. False if not.
   */
  bool run();

protected:
  // Owned state.

  // Borrowed state.
  llvm::Module &M;
  arcana::noelle::Noelle &noelle;
  CollectionAnalysis &CA;
  SizeAnalysis &SizeA;

  set<SequenceAllocInst *> size_variant_sequence_allocations;
  map<SequenceAllocInst *, set<JoinInst *>> alloc_to_join_insts;
  map<SequenceAllocInst *, set<SliceInst *>> alloc_to_slice_insts;

  /**
   * Analyzes the module that was passed into the constructor.
   *
   * returns True if analysis succeeded. False if analysis failed.
   */
  bool analyze();

  /**
   * Transforms the module that was passed into the constructor.
   *
   * returns True if transformation succeeded. False if transformation failed.
   */
  bool transform();
};

} // namespace llvm::memoir

#endif
