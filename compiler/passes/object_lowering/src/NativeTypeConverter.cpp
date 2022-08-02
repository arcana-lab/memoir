//
// Created by Peter Zhong on 7/19/22.
//

#include "NativeTypeConverter.h"

namespace object_lowering {
    NativeTypeConverter::NativeTypeConverter(Module &M, Noelle *noelle) : M(M),
                                                                          noelle(noelle) {

    }

    //for reference types it returns i8*
    llvm::Type *NativeTypeConverter::getLLVMRepresentation(llvm::memoir::TypeSummary &ts)
    {
        if(cache.find(&ts)!= cache.end())
        {
            return cache.at(&ts);
        }
        llvm::Type * created;
        switch(ts.getCode())
        {
            case memoir::StructTy:
                created = getLLVMRepresentation(static_cast<memoir::StructTypeSummary&> (ts));
                break;
            case memoir::TensorTy: {
//                assert(false && "tensor don't really have an llvm correspondance");
                auto tensorType = static_cast< memoir::TensorTypeSummary &> (ts);
                created = getLLVMRepresentation(tensorType.getElementType());
                break;
            }
            case memoir::IntegerTy: {
                auto intType = static_cast< memoir::IntegerTypeSummary &> (ts);
                auto bitwidth = intType.getBitWidth();
                created = llvm::IntegerType::get(M.getContext(), bitwidth);
                break;
            }
            case memoir::FloatTy:{
                created = llvm::Type::getFloatTy(M.getContext());
                break;
            }
            case memoir::DoubleTy:
                created = llvm::Type::getDoubleTy(M.getContext());
                break;
            case memoir::ReferenceTy:
                created = llvm::PointerType::getUnqual(llvm::IntegerType::get(M.getContext(), 8));
                break;
        }

        cache[&ts]= created;
        return created;
    }

    llvm::StructType *NativeTypeConverter::getLLVMRepresentation(llvm::memoir::StructTypeSummary &sts) {
        errs() << sts.toString() << "\n";
        // create llvm::StructType
        std::vector<llvm::Type *> types;
        auto numFields = sts.getNumFields();
        for(uint64_t fieldI =0; fieldI < numFields; fieldI++)
        {
            auto &fieldType = sts.getField(fieldI);
            types.push_back(getLLVMRepresentation(fieldType));
        }
        auto created = llvm::StructType::create(M.getContext(), types, "memoirStruct"+sts.getName(), false);
        return created;
    }
}