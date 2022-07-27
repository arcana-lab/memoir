//
// Created by Peter Zhong on 7/19/22.
//

#include <llvm/IR/DerivedTypes.h>
#include "common/analysis/TypeAnalysis.hpp"
#include "noelle/core/Noelle.hpp"


#ifndef OBJECTLOWERING_NATIVETYPECONVERTER_H
#define OBJECTLOWERING_NATIVETYPECONVERTER_H

namespace object_lowering {
    class NativeTypeConverter {
    public:
        NativeTypeConverter(Module &M, Noelle *noelle);
        Type *getLLVMRepresentation(memoir::TypeSummary &ts);
    private:
        Module &M;
        Noelle *noelle;
        std::map<llvm::memoir::TypeSummary*, llvm::Type*> cache;
        llvm::StructType* getLLVMRepresentation(llvm::memoir::StructTypeSummary& sts);

    };
}


#endif //OBJECTLOWERING_NATIVETYPECONVERTER_H
