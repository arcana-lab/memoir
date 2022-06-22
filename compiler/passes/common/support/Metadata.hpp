#ifndef COMMON_METADATA_H
#define COMMON_METADATA_H
#pragma once

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include <unordered_map>

namespace llvm {
namespace memoir {
/*
 * Metadata types
 *
 * If you update this, make sure you
 * update the metadataToString map
 */
enum MetadataType {
  INTERNAL,
};

class MetadataManager {
public:
  /*
   * Metadata management
   */
  void setMetadata(Function &F, MetadataType MT);

  bool hasMetadata(Function &F, MetadataType MT);

  void setMetadata(Instruction &I, MetadataType MT);

  bool hasMetadata(Instruction &I, MetadataType MT);

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
  void setMetadata(Function &F, StringRef str);

  bool hasMetadata(Function &F, StringRef str);

  void setMetadata(Instruction &I, StringRef str);

  bool hasMetadata(Instruction &I, StringRef str);

  std::unordered_map<MetadataType, StringRef> MDtoString;

  /*
   * Singleton
   */
  MetadataManager()
    : MDtoString{
        { MetadataType::INTERNAL, "memoir.internal" },
      } {}
};

} // namespace memoir
} // namespace llvm

#endif
