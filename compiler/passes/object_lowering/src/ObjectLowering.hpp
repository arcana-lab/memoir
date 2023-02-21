#pragma once

#include <utility>
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Dominators.h"
#include "NativeTypeConverter.hpp"
#include <functional>

namespace object_lowering {

    class ObjectLowering {
    public:
        ObjectLowering(llvm::Module &M, llvm::ModulePass *mp);

        void transform();

        void function_transform(llvm::Function *f);

        void BasicBlockTransformer(llvm::DominatorTree &DT,
                                   llvm::BasicBlock *bb,
                                   std::map<llvm::Value *, llvm::Value *> &replacementMapping,
                                   std::set<llvm::memoir::ControlPHIStruct*> & phiNodesReplacementStruct,
                                   std::set<llvm::memoir::ControlPHICollection*> & phiNodesReplacementCollection);

        llvm::Value * FindBasePointerForStruct(
                llvm::memoir::Struct* structref,
                std::map<llvm::Value *, llvm::Value *> &replacementMapping,
                std::set<llvm::memoir::ControlPHIStruct*> & phiNodesReplacement);

        llvm::Value* FindBasePointerForTensor(
                llvm::memoir::Collection* collection_origin,
                std::map<llvm::Value *, llvm::Value *> &replacementMapping,
                std::set<llvm::memoir::ControlPHICollection*> & phiNodesReplacement
        );

        llvm::Value* GetGEPForTensorUse( llvm::memoir::MemOIRInst* access_ins,
                                         std::map<llvm::Value *, llvm::Value *> &replacementMapping,
                                         std::set<llvm::memoir::ControlPHICollection*> & phiNodesReplacement);
    private:
        llvm::Module &M;
        llvm::ModulePass *mp;
        NativeTypeConverter* nativeTypeConverter;
        std::set<llvm::Value*> toDeletes;
    };
}
