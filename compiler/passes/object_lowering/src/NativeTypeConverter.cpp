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
        if(cache.find(sts)!= cache.end())
        {
            return cache.at(sts);
        }

        // create llvm::StructType
        std::vector<llvm::Type *> types;
        auto numFields = sts->getNumFields();
        for(uint64_t fieldI =0; fieldI < numFields; fieldI++)
        {
            auto fieldType = sts->getField(fieldI);
            switch(fieldType->getCode())
            {
                case llvm::memoir::ReferenceTy:{
                    types.push_back(llvm::PointerType::getUnqual(llvm::IntegerType::get(M.getContext(), 8)));
                    break;
                }
                case llvm::memoir::IntegerTy: {
                    auto intType = (memoir::IntegerTypeSummary *) fieldType;
                    auto bitwidth = intType->getBitWidth();
                    types.push_back(llvm::IntegerType::get(M.getContext(), bitwidth));
                    break;
                }
                case memoir::StructTy: {
                    types.push_back(getLLVMRepresentation((memoir::StructTypeSummary*) fieldType)));
                    break;
                }
                case memoir::TensorTy: {
                    //TODO:
                    assert(false);
                    break;
                }
                case memoir::FloatTy:
                    //TODO:
                    assert(false);
                    break;
                case memoir::DoubleTy:
                    //TODO:
                    assert(false);
                    break;
            }
        }
        auto created = llvm::StructType::create(M.getContext(), types, "my_struct", false);
        cache[sts]= created;
        return created;
    }
}