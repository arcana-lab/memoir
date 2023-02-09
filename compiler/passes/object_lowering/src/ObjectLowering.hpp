#pragma once

#include <utility>
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Dominators.h"
#include "NativeTypeConverter.hpp"

namespace object_lowering {

    class ObjectLowering {
    public:
        ObjectLowering(llvm::Module &M);

        void transform();

        void function_transform(llvm::Function *f);

        void BasicBlockTransformer(llvm::DominatorTree &DT,
                                   llvm::BasicBlock *bb,
                                   std::map<llvm::Value *, llvm::Value *> &replacementMapping,
                                   std::map<llvm::PHINode*, llvm::memoir::ControlPHIStruct*> & phiNodesReplacement);

        llvm::Value * FindBasePointerForStruct(
                llvm::memoir::Struct* structref,
                std::map<llvm::Value *, llvm::Value *> &replacementMapping,
                std::map<llvm::PHINode*, llvm::memoir::ControlPHIStruct*> & phiNodesReplacement);

    private:
        llvm::Module &M;
        NativeTypeConverter* nativeTypeConverter;
    };
}
