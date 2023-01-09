#include "memoir/ir/Instructions.hpp"

namespace llvm::memoir {

MemOIRInst *MemOIRInst::get(llvm::Instruction &I) {
  auto call_inst = dyn_cast<llvm::CallInst>(&I);
  if (!call_inst) {
    return nullptr;
  }

  if (!FunctionNames::is_memoir_call(*call_inst)) {
    return nullptr;
  }

  auto memoir_enum = FunctionNames::get_memoir_enum(*call_inst);

  /*
   * Check if there is an existing MemOIRInst.
   */
  auto found = MemOIRInst::llvm_to_memoir.find(&I);
  if (found != MemOIRInst::llvm_to_memoir.end()) {
    auto &found_inst = *(found->second);
    if (found_inst.getKind() == memoir_enum) {
      return &found_inst;
    }

    /*
     * If the enums don't match, erase the existing.
     * TODO: actually delete the found instruction
     */
    MemOIRInst::llvm_to_memoir.erase(found);
  }

  switch (memoir_enum) {
    default:
      MEMOIR_UNREACHABLE("Unknown MemOIR instruction encountered");
#define HANDLE_INST(ENUM, FUNC, CLASS)                                         \
  case MemOIR_Func::ENUM: {                                                    \
    auto memoir_inst = new CLASS(*call_inst);                                  \
    MemOIRInst::llvm_to_memoir[&I] = memoir_inst;                              \
    return memoir_inst;                                                        \
  }
#include "memoir/ir/Instructions.def"
  }

  return nullptr;
}

map<llvm::Instruction *, MemOIRInst *> MemOIRInst::llvm_to_memoir = {};

/*
 * Top-level methods
 */
MemOIRFunction &MemOIRInst::getFunction() const {
  auto bb = this->getCallInst().getParent();
  MEMOIR_ASSERT(
      (bb != nullptr),
      "Attempt to get function of instruction that has no basic block parent");

  auto func = bb->getParent();
  MEMOIR_ASSERT((func != nullptr),
                "Attempt to get MemOIRFunction for NULL function");

  return MemOIRFunction::get(*func);
}

llvm::CallInst &MemOIRInst::getCallInst() const {
  return this->call_inst;
}

MemOIR_Func MemOIRInst::getKind() const {
  return FunctionNames::get_memoir_enum(this->getCallInst());
}

/*
 * Constructors
 */
MemOIRInst::MemOIRInst(llvm::CallInst &call_inst) : call_inst(call_inst) {
  // Do nothing.
}

TypeInst::TypeInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst) {
  // Do nothing.
}

UInt64TypeInst::UInt64TypeInst(llvm::CallInst &call_inst)
  : TypeInst(call_inst) {
  // Do nothing.
}

UInt32TypeInst::UInt32TypeInst(llvm::CallInst &call_inst)
  : TypeInst(call_inst) {
  // Do nothing.
}

UInt16TypeInst::UInt16TypeInst(llvm::CallInst &call_inst)
  : TypeInst(call_inst) {
  // Do nothing.
}

UInt8TypeInst::UInt8TypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst) {
  // Do nothing.
}

Int64TypeInst::Int64TypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst) {
  // Do nothing.
}

Int32TypeInst::Int32TypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst) {
  // Do nothing.
}

Int16TypeInst::Int16TypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst) {
  // Do nothing.
}

Int8TypeInst::Int8TypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst) {
  // Do nothing.
}

BoolTypeInst::BoolTypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst) {
  // Do nothing.
}

FloatTypeInst::FloatTypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst) {
  // Do nothing.
}

DoubleTypeInst::DoubleTypeInst(llvm::CallInst &call_inst)
  : TypeInst(call_inst) {
  // Do nothing.
}

PointerTypeInst::PointerTypeInst(llvm::CallInst &call_inst)
  : TypeInst(call_inst) {
  // Do nothing.
}

ReferenceTypeInst::ReferenceTypeInst(llvm::CallInst &call_inst)
  : TypeInst(call_inst) {
  // Do nothing.
}

DefineStructTypeInst::DefineStructTypeInst(llvm::CallInst &call_inst)
  : TypeInst(call_inst) {
  // Do nothing.
}

StructTypeInst::StructTypeInst(llvm::CallInst &call_inst)
  : TypeInst(call_inst) {
  // Do nothing.
}

StaticTensorTypeInst::StaticTensorTypeInst(llvm::CallInst &call_inst)
  : TypeInst(call_inst) {
  // Do nothing.
}

TensorTypeInst::TensorTypeInst(llvm::CallInst &call_inst)
  : TypeInst(call_inst) {
  // Do nothing.
}

AssocArrayTypeInst::AssocArrayTypeInst(llvm::CallInst &call_inst)
  : TypeInst(call_inst) {
  // Do nothing.
}

SequenceTypeInst::SequenceTypeInst(llvm::CallInst &call_inst)
  : TypeInst(call_inst) {
  // Do nothing.
}

AllocInst::AllocInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst) {
  // Do nothing.
}

StructAllocInst::StructAllocInst(llvm::CallInst &call_inst)
  : AllocInst(call_inst) {
  // Do nothing.
}

CollectionAllocInst::CollectionAllocInst(llvm::CallInst &call_inst)
  : AllocInst(call_inst) {
  // Do nothing.
}

TensorAllocInst::TensorAllocInst(llvm::CallInst &call_inst)
  : CollectionAllocInst(call_inst) {
  // Do nothing.
}

AssocArrayAllocInst::AssocArrayAllocInst(llvm::CallInst &call_inst)
  : CollectionAllocInst(call_inst) {
  // Do nothing.
}

SequenceAllocInst::SequenceAllocInst(llvm::CallInst &call_inst)
  : CollectionAllocInst(call_inst) {
  // Do nothing.
}

AccessInst::AccessInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst) {
  // Do nothing.
}

ReadInst::ReadInst(llvm::CallInst &call_inst) : AccessInst(call_inst) {
  // Do nothing.
}

StructReadInst::StructReadInst(llvm::CallInst &call_inst)
  : ReadInst(call_inst) {
  // Do nothing.
}

IndexReadInst::IndexReadInst(llvm::CallInst &call_inst) : ReadInst(call_inst) {
  // Do nothing.
}

AssocReadInst::AssocReadInst(llvm::CallInst &call_inst) : ReadInst(call_inst) {
  // Do nothing.
}

WriteInst::WriteInst(llvm::CallInst &call_inst) : AccessInst(call_inst) {
  // Do nothing.
}

StructWriteInst::StructWriteInst(llvm::CallInst &call_inst)
  : WriteInst(call_inst) {
  // Do nothing.
}

IndexWriteInst::IndexWriteInst(llvm::CallInst &call_inst)
  : WriteInst(call_inst) {
  // Do nothing.
}

AssocWriteInst::AssocWriteInst(llvm::CallInst &call_inst)
  : WriteInst(call_inst) {
  // Do nothing.
}

GetInst::GetInst(llvm::CallInst &call_inst) : AccessInst(call_inst) {
  // Do nothing.
}

StructGetInst::StructGetInst(llvm::CallInst &call_inst) : GetInst(call_inst) {
  // Do nothing.
}

IndexGetInst::IndexGetInst(llvm::CallInst &call_inst) : GetInst(call_inst) {
  // Do nothing.
}

AssocGetInst::AssocGetInst(llvm::CallInst &call_inst) : GetInst(call_inst) {
  // Do nothing.
}

DeleteStructInst::DeleteStructInst(llvm::CallInst &call_inst)
  : MemOIRInst(call_inst) {
  // Do nothing.
}

DeleteCollectionInst::DeleteCollectionInst(llvm::CallInst &call_inst)
  : MemOIRInst(call_inst) {
  // Do nothing.
}

JoinInst::JoinInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst) {
  // Do nothing.
}

SliceInst::SliceInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst) {
  // Do nothing.
}

AssertStructTypeInst::AssertStructTypeInst(llvm::CallInst &call_inst)
  : MemOIRInst(call_inst) {
  // Do nothing.
}

AssertCollectionTypeInst::AssertCollectionTypeInst(llvm::CallInst &call_inst)
  : MemOIRInst(call_inst) {
  // Do nothing.
}

ReturnTypeInst::ReturnTypeInst(llvm::CallInst &call_inst)
  : MemOIRInst(call_inst) {
  // Do nothing.
}

} // namespace llvm::memoir
