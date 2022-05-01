#ifndef OBJECTLOWERING_TYPES_H
#define OBJECTLOWERING_TYPES_H
#pragma once

#include <string>
#include <vector>
#include <llvm/IR/Value.h>
#include <llvm/IR/DerivedTypes.h>
#include "llvm/Support/raw_ostream.h"
#include <llvm/IR/Module.h>

namespace object_lowering {

    enum TypeCode {
        ObjectTy,
        ArrayTy,
        UnionTy,
        IntegerTy,
        FloatTy,
        DoubleTy,
    };

    struct AnalysisType {
    protected:
        TypeCode code;

    public:
        TypeCode getCode();

        AnalysisType(TypeCode code);
        ~AnalysisType();

        virtual std::string toString() = 0;

        friend class Object;
        friend class Field;
    };

    struct ObjectType : public AnalysisType {
        std::vector<AnalysisType *> fields;
        llvm::StructType* created = nullptr;
        ObjectType();
        ~ObjectType();

        std::string toString();
        llvm::StructType* getLLVMRepresentation(llvm::Module& m);
    };

    struct ArrayType : public AnalysisType {
        AnalysisType *elementType;

        ArrayType(AnalysisType *elementType);
        ~ArrayType();

        std::string toString();
    };

    struct UnionType : public AnalysisType {
        std::vector<AnalysisType *> members;

        UnionType();
        ~UnionType();

        std::string toString();
    };

    struct IntegerType : public AnalysisType {
        uint64_t bitwidth;
        bool isSigned;

        IntegerType(uint64_t bitwidth, bool isSigned);
        ~IntegerType();

        std::string toString();
    };

    struct FloatType : public AnalysisType {
        FloatType();
        ~FloatType();

        std::string toString();
    };

    struct DoubleType : public AnalysisType {
        DoubleType();
        ~DoubleType();

        std::string toString();
    };

    // new abstractions for ObjectLowering pass

    struct ObjectWrapper{
        explicit ObjectWrapper(ObjectType*);
        ObjectType* innerType;
    };

    struct FieldWrapper{
        int fieldIndex;
        ObjectType* objectType;
        llvm::Value* baseObjPtr; // pointer to the malloc, phi, etc value that this field belongs to
    };

} // namespace objectir



#endif //OBJECTLOWERING_TYPES_H
