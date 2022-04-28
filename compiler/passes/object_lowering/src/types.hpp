//
// Created by peter on 4/18/22.
//

#ifndef OBJECTLOWERING_TYPES_H
#define OBJECTLOWERING_TYPES_H
#pragma once

/*
 * Object representation recognizable by LLVM IR
 * This file describes the AnalysisType interface for the
 * object-ir library.
 *
 * Author(s): Tommy McMichen
 * Created: Mar 7, 2022
 */

#include <string>
#include <vector>
#include <llvm/IR/Value.h>
#include <llvm/IR/DerivedTypes.h>
#include "llvm/Support/raw_ostream.h"
#include <llvm/IR/Module.h>

//  (%0 -> ObjectType(int1,int2,int3))
// (%buildobject -> Object(ObjectType(int1,int2,int3))
// (readfield(%buildobject)
// (%read1 -> pointer(int1))

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


    struct ObjectWrapper{
        explicit ObjectWrapper(ObjectType*);
        ObjectType* innerType;
    };

    struct FieldWrapper{
        int fieldIndex;
        ObjectType* objectType;
        llvm::Value* baseObjPtr;
    };

} // namespace objectir



#endif //OBJECTLOWERING_TYPES_H
