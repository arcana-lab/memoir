#include "NativeTypeConverter.hpp"
#include "Utility.h"

namespace object_lowering {
    NativeTypeConverter::NativeTypeConverter(llvm::Module &M) : M(M) {

    }
    llvm::Type * NativeTypeConverter::getLLVMRepresentation(llvm::memoir::Type* type)
    {
        if(cache.find(type)!= cache.end())
        {
            return cache.at(type);
        }
        llvm::Type * created;
        switch(type->getCode())
        {
            case llvm::memoir::TypeCode::INTEGER: {
                auto intType = static_cast<llvm::memoir::IntegerType *> (type);
                auto bitwidth = intType->getBitWidth();
                created = llvm::IntegerType::get(M.getContext(), bitwidth);
                break;
            }
            case llvm::memoir::TypeCode::FLOAT: {
                created = llvm::Type::getFloatTy(M.getContext());
                break;
            }
            case llvm::memoir::TypeCode::DOUBLE: {
                created = llvm::Type::getDoubleTy(M.getContext());
                break;
            }
            case llvm::memoir::TypeCode::REFERENCE:
            {
                auto referenceType = static_cast<llvm::memoir::ReferenceType *> (type);
                auto referencedType = this->getLLVMRepresentation(&referenceType->getReferencedType());
                created = llvm::PointerType::getUnqual(referencedType);
                break;
            }
            case llvm::memoir::TypeCode::POINTER: {
                created = llvm::PointerType::getUnqual(llvm::IntegerType::get(M.getContext(), 8));
                break;
            }
            case llvm::memoir::TypeCode::STRUCT: {
                created = getLLVMRepresentation(static_cast<llvm::memoir::StructType *> (type));
                break;
            }
            case llvm::memoir::TypeCode::FIELD_ARRAY:
            case llvm::memoir::TypeCode::ASSOC_ARRAY:
            case llvm::memoir::TypeCode::SEQUENCE: {
                llvm::errs() << "Unsupported type conversion operation for " << type->toString() << "\n";
                assert(false && "Unsupported type conversion operation");
            }
            case llvm::memoir::TypeCode::STATIC_TENSOR: {
                auto tensorType = static_cast<llvm::memoir::StaticTensorType *> (type);
                auto elemTy = getLLVMRepresentation(&tensorType->getElementType());
                auto ndim = tensorType->getNumberOfDimensions();
                auto atype = llvm::ArrayType::get(elemTy, tensorType->getLengthOfDimension(ndim - 1));
                for (auto i = ndim - 2; i >= 0; --i) {
                    atype = llvm::ArrayType::get(atype, tensorType->getLengthOfDimension(i));
                }
                created = atype;
                break;
            }
            //Returning an i8* here, but will have special handling code since the for the special sizing offsets
            case llvm::memoir::TypeCode::TENSOR: {
                created = llvm::PointerType::getUnqual(llvm::IntegerType::get(M.getContext(), 8));
                break;
            }
        }
        cache[type]= created;
        return created;


    }

    llvm::StructType* NativeTypeConverter::getLLVMRepresentation(llvm::memoir::StructType* type){
        std::vector<llvm::Type *> types;
        auto numFields = type->getNumFields();
        for(unsigned int fieldI =0; fieldI < numFields; fieldI++)
        {
            auto fieldType = &type->getFieldType(fieldI);
            types.push_back(getLLVMRepresentation(fieldType));
        }
        auto created = llvm::StructType::create(M.getContext(), types, "memoirStruct"+type->getName(), false);
        Utility::debug() << type->getName() << "has been lowered to " << *created << "\n";
        return created;
    }

}
