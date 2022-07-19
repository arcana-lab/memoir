//
// Created by Peter Zhong on 7/19/22.
//

#include "NativeTypeConverter.h"

namespace object_lowering {
    NativeTypeConverter::NativeTypeConverter(Module &M, Noelle *noelle): M(M),
                                                noelle(noelle)
    {

    }

    llvm::StructType *NativeTypeConverter::getLLVMRepresentation(llvm::memoir::StructTypeSummary *sts) {
        return nullptr;
    }
}