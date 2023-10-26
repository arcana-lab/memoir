#ifndef COMMON_METADATA_H
#define COMMON_METADATA_H
#pragma once

#include <string>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include "memoir/support/InternalDatatypes.hpp"

namespace llvm {
namespace memoir {

/*
 * Metadata types
 *
 * If you update this, make sure you
 * update the metadataToString map
 */
enum class MetadataType {
  MD_INTERNAL,
  MD_USE_PHI,
  MD_DEF_PHI,
};

class MetadataManager {
public:
  /*
   * Metadata management
   */
  static void setMetadata(llvm::Function &F, MetadataType MT);
  static void setMetadata(llvm::Function &F,
                          MetadataType MT,
                          llvm::Value *value);
  static bool hasMetadata(llvm::Function &F, MetadataType MT);
  static llvm::Value *getMetadata(Function &F, MetadataType MT);

  static void setMetadata(llvm::Instruction &I, MetadataType MT);
  static void setMetadata(llvm::Instruction &I,
                          MetadataType MT,
                          llvm::Value *value);
  static bool hasMetadata(llvm::Instruction &I, MetadataType MT);
  static llvm::Value *getMetadata(llvm::Instruction &I, MetadataType MT);

  /*
   * Singleton access
   */
  static MetadataManager &getManager() {
    static MetadataManager manager;

    return manager;
  }

  MetadataManager(const MetadataManager &) = delete;
  void operator=(const MetadataManager &) = delete;

private:
  void setMetadata(llvm::Function &F, std::string str);
  void setMetadata(llvm::Function &F, std::string str, llvm::Value *value);
  bool hasMetadata(llvm::Function &F, std::string str);
  llvm::Value *getMetadata(llvm::Function &F, std::string str);

  void setMetadata(llvm::Instruction &I, std::string str);
  void setMetadata(llvm::Instruction &I, std::string str, llvm::Value *value);
  bool hasMetadata(llvm::Instruction &I, std::string str);
  llvm::Value *getMetadata(llvm::Instruction &I, std::string str);

  map<MetadataType, std::string> MDtoString;

  /*
   * Singleton
   */
  MetadataManager()
    : MDtoString{
        { MetadataType::MD_INTERNAL, "memoir.internal" },
        { MetadataType::MD_USE_PHI, "memoir.use-phi" },
        { MetadataType::MD_DEF_PHI, "memoir.def-phi" },
      } {}
};

} // namespace memoir
} // namespace llvm

#endif
