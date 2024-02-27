#ifndef MEMOIR_COLLECTIONSELECTION_H
#define MEMOIR_COLLECTIONSELECTION_H
#pragma once

#include "memoir/support/Casting.hpp"

namespace llvm::memoir {

class CollectionSelection {
public:
  enum SelectionTy {
    SelBeginSequence = 0,
    SelVector,
    SelEndSequence,
    SelBeginAssoc,
    SelHashtable,
    SelEndAssoc,
  };

  static bool is_sequence(SelectionTy selection) {
    return (selection > SelectionTy::SelBeginSequence
            && selection < SelectionTy::SelEndSequence);
  }

  static bool is_assoc(SelectionTy selection) {
    return (selection > SelectionTy::SelBeginAssoc)
           && (selection < SelectionTy::SelEndAssoc);
  }

  static std::string selection_to_string(SelectionTy selection) {
    switch (selection) {
      default:
        return "";
      case SelVector:
        return "vector";
      case SelHashtable:
        return "hashtable";
    }
  }

  using CollectionToSelectionTy = map<llvm::Value *, set<SelectionTy>>;

  bool modified;

  // Perform collection selection analysis, writing the result to
  // output_selections instead of
  CollectionSelection(CollectionToSelectionTy &output_selections,
                      llvm::Module &M) {
    output_selections = analyze(M);
  }

  CollectionSelection(llvm::Module &M) {
    this->modified = transform(analyze(M));
  }

protected:
  // Select implementations for each collection variable in the program.
  CollectionToSelectionTy analyze(llvm::Module &M) {
    CollectionToSelectionTy selections = {};

    for (auto &F : M) {
      for (auto &BB : F) {
        for (auto &I : BB) {
          if (auto *seq_alloc_inst = into<SequenceAllocInst>(I)) {
            selections[&I].insert(SelectionTy::SelVector);
          } else if (auto *assoc_alloc_inst = into<AssocAllocInst>(I)) {
            selections[&I].insert(SelectionTy::SelHashtable);
          }
        }
      }
    }

    return selections;
  }

  // Update the metadata within the program to hold the selected collection
  // implementations.
  bool transform(CollectionToSelectionTy selections) {
    bool transformed = false;
    // for (auto const &[value, selection_set] : selections) {
    //   if (auto *inst = dyn_cast<llvm::Instruction>(&value)) {
    //     for (auto selection : selection_set) {
    //       MetadataManager::insertMetadata(*inst,
    //                                       selection_to_string(selection));
    //     }
    //   } else if (auto *arg = dyn_cast<llvm::Argument>(&value)) {
    //     for (auto selection : selection_set) {
    //       // MetadataManager.insertMetadata(*arg,
    //       // selection_to_string(selection));
    //     }
    //   }
    // }

    return transformed;
  }
};

} // namespace llvm::memoir

#endif
