#ifndef CONSTPROP_H
#define CONSTPROP_H
#include "llvm/IR/Constant.h"
#include <optional>
#pragma once

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include "noelle/core/Noelle.hpp"

#include "memoir/analysis/CollectionAnalysis.hpp"
#include "memoir/analysis/ValueExpression.hpp"
#include "memoir/analysis/ValueNumbering.hpp"
#include "memoir/ir/Collections.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/utility/Metadata.hpp"
#include <unordered_set>

/*
 * Pass to perform constant propagation on MemOIR collections.
 *
 * Author: Nick Wanninger <ncw@u.northwestern.edu>
 * Created: April 17, 2023
 */

using namespace llvm;

namespace constprop {

/// Location - indicates a place in a collection that a value could be stored to
/// or read from. At its core this class is analogous to a GEP instruction in
/// LLVM's IR, as it represents a location in a collection that can be written
/// to or read from. The values in the dataflow for this constant propagation
/// are a tuple of (Location, Constant) which represents the idea that at each
/// instruction in the program, a set of Locations can have a constant value. If
/// you attempt to read from one of those constant values, you can simply
/// propagate the constant from that tuple. If you write to a Location, the
/// dataflow simply kills all Locations from that point forward. To accomplish
/// that, this class relies on MemOIR's ValueExpression system to determine
/// equality of several location.
class Location {
public:
  /// Get the collection for this Location
  memoir::Collection &getCollection(void) const;
  /// Get the number of indicies used in this Location
  unsigned getNumInds(void) const;
  /// Get a specific index into the collection
  memoir::ValueExpression *getInd(unsigned i) const;

  Location(memoir::Collection &c,
           SmallVector<memoir::ValueExpression *, 8> &&inds);

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const Location &L) {
    // I know this isn't kosher printing style, but I just need it to debug :^)
    os << "Location:\n";
    os << "  Collection: " << L.getCollection() << "\n";
    os << "  Inds:";
    for (int i = 0; i < L.getNumInds(); i++) {
      os << " (";
      os << *L.getInd(i) << ", ";
      L.getInd(i)->getValue()->printAsOperand(os);
      os << ")";
    }
    os << "\n";
    return os;
  }

private:
  memoir::Collection &m_collection;
  SmallVector<memoir::ValueExpression *, 8> m_inds;
};

class ConstantPropagation {
public:
  ConstantPropagation(Module &M, memoir::CollectionAnalysis &CA);

  /// Analyze the program
  void analyze();

  /// Transform the program
  void transform();

private:
  Module &M;
  memoir::CollectionAnalysis &CA;
  memoir::ValueNumbering VN;

  // All the locations this analysis has found. The goal here is to have pointer
  // identity be possible for certain Location instances, and to reduce the
  // overall memory impact of these locations, as there could be thousands in a
  // module. The dataflow engine holds raw pointers to these.
  set<unique_ptr<Location>> locations;

  // Dataflow:
  map<llvm::Instruction *, set<Location *>> GEN;
};

} // namespace constprop

#endif
