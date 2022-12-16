#include "common/ir/Instructions.hpp"

namespace llvm::memoir {

/*
 * Constructors
 */
MemOIRInst::MemOIRInst(MemOIR_Func memoir_enum, llvm::CallInst &call_inst)
  : memoir_enum(memoir_enum),
    call_inst(call_inst) {
  // Do nothing.
}

TypeInst::TypeInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst) {
  // Do nothing.
}

IntegerTypeInst::IntegerTypeInst(llvm::CallInst &call_inst)
  : TypeInst(call_inst) {
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

TensorAllocInst::TensorAllocInst(llvm::CallInst &call_inst)
  : AllocInst(call_inst) {
  // Do nothing.
}

AssocArrayAllocInst::AssocArrayAllocInst(llvm::CallInst &call_inst)
  : AllocInst(call_inst) {
  // Do nothing.
}

SequenceAllocInst::SequenceAllocInst(llvm::CallInst &call_inst)
  : AllocInst(call_inst) {
  // Do nothing.
}

AccessInst::AccessInst(llvm::CallInst &call_inst,
                       AccessInfo access_info,
                       IndexInfo index_info)
  : access_info(access_info),
    index_info(index_info),
    MemOIRInst(call_inst) {
  // Do nothing.
}

ReadInst::ReadInst(llvm::CallInst &call_inst, AccessInst::IndexInfo index_info)
  : AccessInst(call_inst, AccessInst::AccessInfo::READ, index_info) {
  // Do nothing.
}

StructReadInst::StructReadInst(llvm::CallInst &call_inst)
  : ReadInst(call_inst, AccessInst::IndexInfo::STRUCT) {
  // Do nothing.
}

IndexReadInst::IndexReadInst(llvm::CallInst &call_inst)
  : ReadInst(call_inst, AccessInst::IndexInfo::INDEX) {
  // Do nothing.
}

AssocReadInst::AssocReadInst(llvm::CallInst &call_inst)
  : ReadInst(call_inst, AccessInst::IndexInfo::ASSOC) {
  // Do nothing.
}

WriteInst::WriteInst(llvm::CallInst &call_inst,
                     AccessInst::IndexInfo index_info)
  : AccessInst(call_inst, AccessInst::AccessInfo::WRITE, index_info) {
  // Do nothing.
}

StructReadInst::StructReadInst(llvm::CallInst &call_inst)
  : ReadInst(call_inst, AccessInst::IndexInfo::STRUCT) {
  // Do nothing.
}

IndexReadInst::IndexReadInst(llvm::CallInst &call_inst)
  : ReadInst(call_inst, AccessInst::IndexInfo::INDEX) {
  // Do nothing.
}

AssocReadInst::AssocReadInst(llvm::CallInst &call_inst)
  : ReadInst(call_inst, AccessInst::IndexInfo::ASSOC) {
  // Do nothing.
}

GetInst::GetInst(llvm::CallInst &call_inst, AccessInst::IndexInfo index_info)
  : AccessInst(call_inst, AccessInst::AccessInfo::GET, index_info) {
  // Do nothing.
}

StructGetInst::StructGetInst(llvm::CallInst &call_inst)
  : GetInst(call_inst, AccessInst::IndexInfo::STRUCT) {
  // Do nothing.
}

IndexGetInst::IndexGetInst(llvm::CallInst &call_inst)
  : GetInst(call_inst, AccessInst::IndexInfo::INDEX) {
  // Do nothing.
}

AssocGetInst::AssocGetInst(llvm::CallInst &call_inst)
  : GetInst(call_inst, AccessInst::IndexInfo::ASSOC) {
  // Do nothing.
}

DeleteInst::DeleteInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst) {
  // Do nothing.
}

JoinInst::JoinInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst) {
  // Do nothing.
}

SliceInst::SliceInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst) {
  // Do nothing.
}

AssertTypeInst::AssertTypeInst(llvm::CallInst &call_inst)
  : MemOIRInst(call_inst) {
  // Do nothing.
}

ReturnTypeInst::ReturnTypeInst(llvm::CallInst &call_inst)
  : MemOIRInst(call_inst) {
  // Do nothing.
}

} // namespace llvm::memoir
