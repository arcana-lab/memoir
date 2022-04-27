//
// Created by peter on 4/18/22.
//

#include "types.hpp"


using namespace object_lowering;

TypeCode AnalysisType::getCode() {
    return this->code;
}

/*
 * AnalysisType base class
 */
AnalysisType::AnalysisType(TypeCode code) : code(code) {
    // Do nothing
}

AnalysisType::~AnalysisType() {
    // Do nothing
}

/*
 * Object AnalysisType
 */
ObjectType::ObjectType() : AnalysisType(TypeCode::ObjectTy) {
    // Do nothing.
}

ObjectType::~ObjectType() {
    for (auto field : this->fields) {
        delete field;
    }
}

/*
 * Array AnalysisType
 */
ArrayType::ArrayType(AnalysisType *type)
        : AnalysisType(TypeCode::ArrayTy),
          elementType(type) {
    // Do nothing.
}

ArrayType::~ArrayType() {
    delete elementType;
}

/*
 * Union AnalysisType
 */
UnionType::UnionType() : AnalysisType(TypeCode::UnionTy) {
    // Do nothing.
}

UnionType::~UnionType() {
    for (auto member : this->members) {
        delete member;
    }
}

/*
 * Integer AnalysisType
 */
IntegerType::IntegerType(uint64_t bitwidth, bool isSigned)
        : AnalysisType(TypeCode::IntegerTy),
          bitwidth(bitwidth),
          isSigned(isSigned) {
    // Do nothing.
}
IntegerType::~IntegerType() {
    // Do nothing
}

/*
 * Float AnalysisType
 */
FloatType::FloatType() : AnalysisType(TypeCode::FloatTy) {
    // Do nothing.
}

FloatType::~FloatType() {
    // Do nothing;
}


/*
 * Double AnalysisType
 */
DoubleType::DoubleType() : AnalysisType(TypeCode::DoubleTy) {
    // Do nothing.
}

DoubleType::~DoubleType() {
    // Do nothing.
}

ObjectWrapper::ObjectWrapper(ObjectType * it) {
    innerType = it;
}


std::string ObjectType::toString() {
    std::string str = "(Object: \n";
    for (auto field : this->fields) {
        str += "  (Field: ";
        str += field->toString();
        str += ")\n";
    }
    str += ")\n";
    return str;
}

llvm::StructType* ObjectType::getLLVMRepresentation(llvm::Module& M) {
    std::vector<llvm::Type *> types;

    for (auto fieldType: this->fields) {
        switch (fieldType->getCode()) {
            case IntegerTy: {
                auto intType = (IntegerType *) fieldType;
                //TODO: Not every int is 64 bit but for all assume 64 bits
                types.push_back(llvm::Type::getInt64Ty(M.getContext()));
                break;
            }
            case ObjectTy: {
                types.push_back(llvm::PointerType::getUnqual(llvm::IntegerType::get(M.getContext(), 8)));
                break;
            }
            //TODO: other cases
            default:
                assert(false);
        }
    }

    return llvm::StructType::create(M.getContext(), types, "my_struct", false);
}



std::string ArrayType::toString() {
    return "AnalysisType: array";
}

std::string UnionType::toString() {
    return "AnalysisType: union";
}

std::string IntegerType::toString() {
    return "AnalysisType: integer";
}

std::string FloatType::toString() {
    return "AnalysisType: float";
}

std::string DoubleType::toString() {
    return "AnalysisType: double";
}

