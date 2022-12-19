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
enum MetadataType {
  INTERNAL,
};

class MetadataManager {
public:
  /*
   * Metadata management
   */
  static void setMetadata(Function &F, MetadataType MT);

  static bool hasMetadata(Function &F, MetadataType MT);

  static void setMetadata(Instruction &I, MetadataType MT);

  static bool hasMetadata(Instruction &I, MetadataType MT);

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
  void setMetadata(Function &F, std::string str);

  bool hasMetadata(Function &F, std::string str);

  void setMetadata(Instruction &I, std::string str);

  bool hasMetadata(Instruction &I, std::string str);

  map<MetadataType, std::string> MDtoString;

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
