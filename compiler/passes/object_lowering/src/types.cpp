#include "types.hpp"

using namespace object_lowering;

TypeCode AnalysisType::getCode() {
    return this->code;
}

/*
 * AnalysisType base class
 */
AnalysisType::AnalysisType(TypeCode code) : code(code) { }

AnalysisType::~AnalysisType() { }

/*
 * Object AnalysisType
 */
ObjectType::ObjectType() : AnalysisType(TypeCode::ObjectTy) { }

ObjectType::~ObjectType() {
    for (auto field : this->fields) {
        delete field;
    }
}

// Pointer AnalysisType
APointerType::APointerType() : AnalysisType(TypeCode::PointerTy) { }

APointerType::~APointerType() { }

StubType::StubType(std::string name0) : AnalysisType(TypeCode::StubTy) { name = name0; }

StubType::~StubType() { }

/*
 * Array AnalysisType
 */
ArrayType::ArrayType(AnalysisType *type)
        : AnalysisType(TypeCode::ArrayTy),
          elementType(type) { }

ArrayType::~ArrayType() {
    delete elementType;
}

/*
 * Union AnalysisType
 */
UnionType::UnionType() : AnalysisType(TypeCode::UnionTy) { }

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
          isSigned(isSigned) { }
IntegerType::~IntegerType() { }

/*
 * Float AnalysisType
 */
FloatType::FloatType() : AnalysisType(TypeCode::FloatTy) { }
FloatType::~FloatType() { }


/*
 * Double AnalysisType
 */
DoubleType::DoubleType() : AnalysisType(TypeCode::DoubleTy) { }

DoubleType::~DoubleType() { }

// ========================= OBJECT LOWERING ABSTRACTIONS ==========================================

ObjectWrapper::ObjectWrapper(ObjectType * it) {
    innerType = it;
}

llvm::StructType* ObjectType::getLLVMRepresentation(llvm::Module& M) {
    llvm::errs() << "the current created type cached is " << created << "\n";
    if(created != nullptr)
    {
        llvm::errs() << "and it's not a nullptr " << *created << "\n\n";
    }

    // return the cached type
    if (created!=nullptr) return created;

    // create llvm::StructType
    std::vector<llvm::Type *> types;

    for (auto fieldType: this->fields) {
        switch (fieldType->getCode()) {
            case PointerTy: {
                types.push_back(llvm::PointerType::getUnqual(llvm::IntegerType::get(M.getContext(), 8)));
                break;
            }
            case IntegerTy: {
                auto intType = (IntegerType *) fieldType;
                switch (intType->bitwidth) {
                    case 64: {
                        types.push_back(llvm::Type::getInt64Ty(M.getContext()));
                        break;
                    }
                    case 32: {
                        types.push_back(llvm::Type::getInt32Ty(M.getContext()));
                        break;
                    }
                    default: {
                        llvm::errs() << "IntegerType: bitwidth " << intType->bitwidth << " not supported\n";
                        assert(false);
                    }
                }
                break;
            }
            // needs  other cases
            default:
                assert(false);
        }
    }
    created = llvm::StructType::create(M.getContext(), types, "my_struct", false);
    return created;
}

/*llvm::StructType* APointerType::getLLVMRepresentation(llvm::Module& M) {
    if (auto objTy = dyn_cast_or_null<ObjectType>(pointsTo)) {
        return objTy->getLLVMRepresentation(M);
    } else {
        errs() << "APointerType::getLLVMRepresentation expected that it points to an object, got: " << pointsTo->toString() << "\n";
        assert(false);
    }
}*/

std::string APointerType::toString() {
    std::string str = "(Ptr: \n";
    if(pointsTo->getCode() == ObjectTy) {
        auto objTy = (ObjectType*) pointsTo;
        if (objTy->hasName()) str += objTy->name;
        else                  str += "unnamed obj";
    }
    str += ")";
    return str;
}

std::string ObjectType::toString() {
    std::string str = "(Object: \n";
    if (hasName()) str += "named: " + name + "\n";
    for (auto field : this->fields) {
        str += "  (Field: ";
        str += field->toString();
        str += ")\n";
    }
    str += ")\n";
    return str;
}

bool ObjectType::hasName() {
    return ! name.empty();
}

std::string StubType::toString() {
    return "Stub: " + name;
}

// ========================= OBJECT LOWERING ABSTRACTIONS ======================================

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

