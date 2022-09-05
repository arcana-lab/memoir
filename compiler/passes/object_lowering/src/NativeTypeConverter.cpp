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
        errs() << "the type below has a type  " << ts.getCode() << "\n";
//        errs() << "getting llvm rep for this type " << ts.toString() << "here \n";
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
//                assert(false && "tensor don't really have a llvm correspondence");
                auto tensorType = static_cast< memoir::TensorTypeSummary &> (ts);
                auto elemTy = getLLVMRepresentation(tensorType.getElementType());
                if(tensorType.isStaticLength()) {
                    auto ndim = tensorType.getNumDimensions();
                    auto atype = ArrayType::get(elemTy, tensorType.getLengthOfDimension(ndim - 1));
                    for (auto i = ndim - 2; i >= 0; --i) {
                        atype = ArrayType::get(atype, tensorType.getLengthOfDimension(i));
                    }
                    created = atype;
                }
                else{
                    created = llvm::PointerType::getUnqual(llvm::IntegerType::get(M.getContext(), 8));
                }
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
        errs() << "outputting type :" << *created <<"\n";
        return created;
    }

    llvm::StructType *NativeTypeConverter::getLLVMRepresentation(llvm::memoir::StructTypeSummary &sts) {
//        errs() << sts.toString() << "\n";
        // create llvm::StructType
        std::vector<llvm::Type *> types;
        auto numFields = sts.getNumFields();
        errs() << "The struct has " << numFields <<"Fields\n";
        for(uint64_t fieldI =0; fieldI < numFields; fieldI++)
        {
            errs() << "checking field " << fieldI <<" of that struct \n";
            auto &fieldType = sts.getField(fieldI);
            types.push_back(getLLVMRepresentation(fieldType));
        }
        auto created = llvm::StructType::create(M.getContext(), types, "memoirStruct"+sts.getName(), false);
        return created;
    }
}