//
// Created by Peter Zhong on 7/19/22.
//

#include "NativeTypeConverter.h"

namespace object_lowering {
    NativeTypeConverter::NativeTypeConverter(Module &M, Noelle *noelle): M(M),
                                                noelle(noelle)
    {

    }

    llvm::StructType *NativeTypeConverter::getLLVMRepresentation(llvm::memoir::StructTypeSummary &sts) {
        if(cache.find(&sts)!= cache.end())
        {
            return cache.at(&sts);
        }

        // create llvm::StructType
        std::vector<llvm::Type *> types;
        auto numFields = sts.getNumFields();
        for(uint64_t fieldI =0; fieldI < numFields; fieldI++)
        {
            auto &fieldType = sts.getField(fieldI);
            switch(fieldType.getCode())
            {
                case llvm::memoir::ReferenceTy:{
                    types.push_back(llvm::PointerType::getUnqual(llvm::IntegerType::get(M.getContext(), 8)));
                    break;
                }
                case llvm::memoir::IntegerTy: {
                    auto intType = static_cast< memoir::IntegerTypeSummary &> (fieldType);
                    auto bitwidth = intType.getBitWidth();
                    types.push_back(llvm::IntegerType::get(M.getContext(), bitwidth));
                    break;
                }
                case memoir::StructTy: {
                    types.push_back(getLLVMRepresentation( static_cast< memoir::StructTypeSummary &> (fieldType)));
                    break;
                }
                case memoir::TensorTy: {
                    //TODO:
                    assert(false);
                    break;
                }
                case memoir::FloatTy:
                    types.push_back(llvm::Type::getFloatTy(M.getContext()));
                    break;
                case memoir::DoubleTy:
                    types.push_back(llvm::Type::getDoubleTy(M.getContext()));
                    break;
            }
        }
        auto created = llvm::StructType::create(M.getContext(), types, "memoirStruct", false);
        cache[&sts]= created;
        return created;
    }
}