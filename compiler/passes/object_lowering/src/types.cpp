//
// Created by peter on 4/18/22.
//

#include "types.hpp"


using namespace object_lowering;

TypeCode Type::getCode() {
    return this->code;
}

/*
 * Type base class
 */
Type::Type(TypeCode code) : code(code) {
    // Do nothing
}

Type::~Type() {
    // Do nothing
}

/*
 * Object Type
 */
ObjectType::ObjectType() : Type(TypeCode::ObjectTy) {
    // Do nothing.
}

ObjectType::~ObjectType() {
    for (auto field : this->fields) {
        delete field;
    }
}

/*
 * Array Type
 */
ArrayType::ArrayType(Type *type)
        : Type(TypeCode::ArrayTy),
          elementType(type) {
    // Do nothing.
}

ArrayType::~ArrayType() {
    delete elementType;
}

/*
 * Union Type
 */
UnionType::UnionType() : Type(TypeCode::UnionTy) {
    // Do nothing.
}

UnionType::~UnionType() {
    for (auto member : this->members) {
        delete member;
    }
}

/*
 * Integer Type
 */
IntegerType::IntegerType(uint64_t bitwidth, bool isSigned)
        : Type(TypeCode::IntegerTy),
          bitwidth(bitwidth),
          isSigned(isSigned) {
    // Do nothing.
}
IntegerType::~IntegerType() {
    // Do nothing
}

/*
 * Float Type
 */
FloatType::FloatType() : Type(TypeCode::FloatTy) {
    // Do nothing.
}

FloatType::~FloatType() {
    // Do nothing;
}


/*
 * Double Type
 */
DoubleType::DoubleType() : Type(TypeCode::DoubleTy) {
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
    return "Type: array";
}

std::string UnionType::toString() {
    return "Type: union";
}

std::string IntegerType::toString() {
    return "Type: integer";
}

std::string FloatType::toString() {
    return "Type: float";
}

std::string DoubleType::toString() {
    return "Type: double";
}

