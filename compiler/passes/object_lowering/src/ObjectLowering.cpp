#include "ObjectLowering.hpp"
#include <functional>
#include "memoir/ir/Instructions.hpp"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "Utility.h"


using namespace llvm;

namespace object_lowering {
    ObjectLowering::ObjectLowering(llvm::Module &M) : M(M) {
        nativeTypeConverter = new NativeTypeConverter(M);
    }

    void ObjectLowering::transform() {

    }

    void ObjectLowering::function_transform(llvm::Function *f) {

    }

    llvm::Value *ObjectLowering::FindBasePointerForStruct(
            memoir::Struct* struct_origin,
            std::map<llvm::Value *, llvm::Value *> &replacementMapping,
            std::map<llvm::PHINode*, memoir::ControlPHIStruct*> & phiNodesReplacement)
    {
        auto struct_code = struct_origin->getCode();
        switch(struct_code)
        {
            case memoir::StructCode::BASE: {
                auto base_struct_origin = static_cast<memoir::BaseStruct*>(struct_origin);
                auto &allocInst = base_struct_origin->getAllocInst();
                return replacementMapping[&allocInst.getCallInst()];
            }
            case memoir::StructCode::CONTAINED:
            case memoir::StructCode::NESTED: {
                //TODO: ask Tommy for help about GEP instruction offsetting.
                auto nested_struct_origin = static_cast<memoir::ContainedStruct*>(struct_origin);
                auto& parent_struct_get = nested_struct_origin->getAccess();
                return replacementMapping[&parent_struct_get.getCallInst()];
            }
            case memoir::StructCode::REFERENCED: {
                auto referenced_struct_origin = static_cast<memoir::ReferencedStruct*>(struct_origin);
                auto& reference_struct_get =referenced_struct_origin->getAccess();
                return replacementMapping[&reference_struct_get.getCallInst()];
            }
            case memoir::StructCode::CONTROL_PHI: {
                auto control_phi_origin = static_cast<memoir::ControlPHIStruct *>(struct_origin);
                auto &original_phi_node = control_phi_origin->getPHI();
                auto struct_type = (memoir::StructType *) &control_phi_origin->getType();
                auto struct_type_as_type = static_cast<memoir::Type *>(struct_type);
                auto llvmType = nativeTypeConverter->getLLVMRepresentation(struct_type_as_type);
                auto llvmPtrType = PointerType::getUnqual(llvmType);
                IRBuilder<> builder(&original_phi_node);
                auto newPhi = builder.CreatePHI(llvmPtrType, original_phi_node.getNumIncomingValues());
                replacementMapping[&original_phi_node] = newPhi;
                phiNodesReplacement[&original_phi_node] = control_phi_origin;
                return newPhi;
            }
            case memoir::StructCode::CALL_PHI:
                assert(false && "TODO for later");
        }

    }


    void ObjectLowering::BasicBlockTransformer(llvm::DominatorTree &DT,
                                               llvm::BasicBlock *bb,
                                               std::map<llvm::Value *, llvm::Value *> &replacementMapping,
                                               std::map<llvm::PHINode*, memoir::ControlPHIStruct*> & phiNodesReplacement) {
        auto &ctxt = M.getContext();
        auto int64Ty = llvm::Type::getInt64Ty(ctxt);
        auto int32Ty = llvm::Type::getInt32Ty(ctxt);
        auto i8Ty = llvm::IntegerType::get(ctxt, 8);
        auto i8StarTy = llvm::PointerType::getUnqual(i8Ty);
        auto voidTy = llvm::Type::getVoidTy(ctxt);
        auto mallocFTY =
                llvm::FunctionType::get(i8StarTy, ArrayRef<Type *>({int64Ty}),
                                        false);
        auto mallocf = M.getOrInsertFunction("malloc", mallocFTY);
        auto freeFTY =
                llvm::FunctionType::get(voidTy, ArrayRef<Type *>({i8StarTy}),
                                        false);

        auto freef = M.getOrInsertFunction("free", freeFTY);
        for (auto &ins: *bb) {
            IRBuilder<> builder(&ins);
            auto mins = memoir::MemOIRInst::get(ins);
            if (mins == nullptr) {
                continue;
            }
            switch (mins->getKind()) {
                case llvm::memoir::DEFINE_STRUCT_TYPE:
                case llvm::memoir::STRUCT_TYPE:
                case llvm::memoir::TENSOR_TYPE:
                case llvm::memoir::STATIC_TENSOR_TYPE:
                case llvm::memoir::ASSOC_ARRAY_TYPE:
                case llvm::memoir::SEQUENCE_TYPE:
                case llvm::memoir::UINT64_TYPE:
                case llvm::memoir::UINT32_TYPE:
                case llvm::memoir::UINT16_TYPE:
                case llvm::memoir::UINT8_TYPE:
                case llvm::memoir::INT64_TYPE:
                case llvm::memoir::INT32_TYPE:
                case llvm::memoir::INT16_TYPE:
                case llvm::memoir::INT8_TYPE:
                case llvm::memoir::BOOL_TYPE:
                case llvm::memoir::FLOAT_TYPE:
                case llvm::memoir::DOUBLE_TYPE:
                case llvm::memoir::POINTER_TYPE:
                case llvm::memoir::REFERENCE_TYPE:
                    break;
                case llvm::memoir::ALLOCATE_STRUCT: {
                    auto allo_struct_ins = static_cast<memoir::StructAllocInst *>(mins);
                    auto &type = allo_struct_ins->getType();
                    auto llvmType = nativeTypeConverter->getLLVMRepresentation(&type);
                    auto llvmTypeSize = llvm::ConstantInt::get(
                            int64Ty,
                            M.getDataLayout().getTypeAllocSize(llvmType));
                    std::vector<Value *> arguments{llvmTypeSize};
                    auto newMallocCall = builder.CreateCall(mallocf, arguments);
                    auto bc_inst =
                            builder.CreateBitCast(newMallocCall,
                                                  PointerType::getUnqual(llvmType));
                    replacementMapping[&ins] = bc_inst;
                    break;
                }
                case llvm::memoir::ALLOCATE_TENSOR: {
                    auto alloc_tensor_ins = static_cast<memoir::TensorAllocInst *>(mins);
                    auto &eletype = alloc_tensor_ins->getElementType();
                    Type *llvmType = nativeTypeConverter->getLLVMRepresentation(&eletype);
                    auto llvmTypeSize = llvm::ConstantInt::get(
                            int64Ty,
                            M.getDataLayout().getTypeAllocSize(llvmType));
                    Value *finalSize;
                    finalSize = llvmTypeSize;
                    auto numdim = alloc_tensor_ins->getNumberOfDimensions();
                    for (unsigned long long i = 0; i < numdim; ++i) {
                        finalSize = builder.CreateMul(finalSize, &alloc_tensor_ins->getLengthOfDimensionOperand(i));
                    }
                    auto int64Size = llvm::ConstantInt::get(int64Ty,
                                                            M.getDataLayout().getTypeAllocSize(int64Ty));

                    auto headerSize = builder.CreateMul(llvm::ConstantInt::get(int64Ty, numdim),
                                                        int64Size);
                    finalSize = builder.CreateAdd(finalSize, headerSize);
                    std::vector<Value *> arguments{finalSize};
                    auto newMallocCall = builder.CreateCall(mallocf, arguments);
                    auto bcInst = builder.CreateBitCast(newMallocCall, i8StarTy);
                    replacementMapping[&ins] = bcInst;
                    for (unsigned long long i = 0; i < numdim; ++i) {
                        std::vector<Value *> indexList{ConstantInt::get(int64Ty, i)};
                        auto gep = builder.CreateGEP(PointerType::getUnqual(int64Ty), newMallocCall,
                                                     indexList);
                        builder.CreateStore(&alloc_tensor_ins->getLengthOfDimensionOperand(i), gep);
                    }
                    break;
                }
                case llvm::memoir::ALLOCATE_ASSOC_ARRAY:
                case llvm::memoir::ALLOCATE_SEQUENCE:
                    errs() << "Assoc array and Seq not supported \n";
                    assert(false);
                case llvm::memoir::STRUCT_READ_UINT64:
                case llvm::memoir::STRUCT_READ_UINT32:
                case llvm::memoir::STRUCT_READ_UINT16:
                case llvm::memoir::STRUCT_READ_UINT8:
                case llvm::memoir::STRUCT_READ_INT64:
                case llvm::memoir::STRUCT_READ_INT32:
                case llvm::memoir::STRUCT_READ_INT16:
                case llvm::memoir::STRUCT_READ_INT8:
                case llvm::memoir::STRUCT_READ_DOUBLE:
                case llvm::memoir::STRUCT_READ_FLOAT:
                case llvm::memoir::STRUCT_READ_STRUCT_REF:
                case llvm::memoir::STRUCT_READ_COLLECTION_REF: {
                    auto struct_read_ins = static_cast<memoir::StructReadInst *>(mins);
                    auto& struct_accessed = struct_read_ins->getStructAccessed();
                    auto base_struct_ptr = FindBasePointerForStruct(&struct_accessed,
                                                                    replacementMapping,
                                                                    phiNodesReplacement);
                    auto field_index = struct_read_ins->getFieldIndex();
                    Value *indices[2] = {llvm::ConstantInt::get(int64Ty, 0), llvm::ConstantInt::get(int64Ty, field_index)};
                    auto gep = builder.CreateGEP(base_struct_ptr,
                                            indices);

                    auto struct_type = (memoir::StructType*) &struct_read_ins->getStructAccessed().getType();
                    auto &field_type = struct_type->getFieldType(field_index);
                    switch(field_type.getCode()) {
                        case memoir::TypeCode::INTEGER:
                        case memoir::TypeCode::FLOAT:
                        case memoir::TypeCode::DOUBLE:
                        case memoir::TypeCode::POINTER: {
                            auto targetType = nativeTypeConverter->getLLVMRepresentation(&field_type);
                            auto loadInst =
                                    builder.CreateLoad(targetType, gep, "baseload");
                            replacementMapping[&ins] = loadInst;
                            ins.replaceAllUsesWith(loadInst);
                            continue;
                        }
                        case memoir::TypeCode::STATIC_TENSOR: {
                            auto static_tensor_type = static_cast<memoir::StaticTensorType *>(&field_type);
                            auto llvm_inner_type = nativeTypeConverter->getLLVMRepresentation((memoir::Type *)
                                                                                                      &static_tensor_type->getElementType());
                            auto bitcast_ins = builder.CreateBitCast(gep, PointerType::getUnqual(llvm_inner_type));
                            replacementMapping[&ins] = bitcast_ins;
                            continue;
                        }
                        case memoir::TypeCode::REFERENCE: {
                            auto reference_type = static_cast<memoir::StaticTensorType *>(&field_type );
                            auto elem_type = (memoir::Type *) &reference_type->getElementType();
                            auto elem_type_llvm = nativeTypeConverter->getLLVMRepresentation(elem_type);
                            auto loadInst =
                                    builder.CreateLoad(llvm::PointerType::getUnqual(elem_type_llvm), gep, "baseload");
                            replacementMapping[&ins] = loadInst;
                            continue;
                        }
                        case memoir::TypeCode::STRUCT: {
                            auto loadInst = builder.CreateLoad(i8StarTy, gep, "baseload");
                            auto inner_struct_type = (memoir::Type *) (&field_type);
                            auto llvm_struct_type = nativeTypeConverter->getLLVMRepresentation(inner_struct_type);
                            auto bitcast = builder.CreateBitCast(gep, PointerType::getUnqual(llvm_struct_type));
                            replacementMapping[&ins] = bitcast;
                            continue;
                        }
                        case memoir::TypeCode::FIELD_ARRAY:
                            assert(false && "Not currently supported");
                        case memoir::TypeCode::TENSOR:
                            assert(false && "only static tensors can be embedded");
                        case memoir::TypeCode::ASSOC_ARRAY:
                        case memoir::TypeCode::SEQUENCE:
                            assert(false && "assoc and sequence should be lowered to tensors first. ");
                    }
                    
                    break;
                }
                case llvm::memoir::STRUCT_GET_STRUCT:
                case llvm::memoir::STRUCT_GET_COLLECTION:
                    break;
                case llvm::memoir::STRUCT_WRITE_UINT64:
                case llvm::memoir::STRUCT_WRITE_UINT32:
                case llvm::memoir::STRUCT_WRITE_UINT16:
                case llvm::memoir::STRUCT_WRITE_UINT8:
                case llvm::memoir::STRUCT_WRITE_INT64:
                case llvm::memoir::STRUCT_WRITE_INT32:
                case llvm::memoir::STRUCT_WRITE_INT16:
                case llvm::memoir::STRUCT_WRITE_INT8:
                case llvm::memoir::STRUCT_WRITE_DOUBLE:
                case llvm::memoir::STRUCT_WRITE_FLOAT:
                case llvm::memoir::STRUCT_WRITE_STRUCT_REF:
                case llvm::memoir::STRUCT_WRITE_COLLECTION_REF:
                    break;
                case llvm::memoir::INDEX_READ_UINT64:
                case llvm::memoir::INDEX_READ_UINT32:
                case llvm::memoir::INDEX_READ_UINT16:
                case llvm::memoir::INDEX_READ_UINT8:
                case llvm::memoir::INDEX_READ_INT64:
                case llvm::memoir::INDEX_READ_INT32:
                case llvm::memoir::INDEX_READ_INT16:
                case llvm::memoir::INDEX_READ_INT8:
                case llvm::memoir::INDEX_READ_DOUBLE:
                case llvm::memoir::INDEX_READ_FLOAT:
                case llvm::memoir::INDEX_READ_STRUCT_REF:
                case llvm::memoir::INDEX_READ_COLLECTION_REF:
                    break;
                case llvm::memoir::ASSOC_READ_UINT64:
                case llvm::memoir::ASSOC_READ_UINT32:
                case llvm::memoir::ASSOC_READ_UINT16:
                case llvm::memoir::ASSOC_READ_UINT8:
                case llvm::memoir::ASSOC_READ_INT8:
                case llvm::memoir::ASSOC_READ_INT16:
                case llvm::memoir::ASSOC_READ_INT64:
                case llvm::memoir::ASSOC_READ_INT32:
                case llvm::memoir::ASSOC_READ_FLOAT:
                case llvm::memoir::ASSOC_READ_DOUBLE:
                case llvm::memoir::ASSOC_READ_STRUCT_REF:
                case llvm::memoir::ASSOC_READ_COLLECTION_REF:
                    assert(false);
                case llvm::memoir::INDEX_WRITE_UINT64:
                case llvm::memoir::INDEX_WRITE_UINT32:
                case llvm::memoir::INDEX_WRITE_UINT16:
                case llvm::memoir::INDEX_WRITE_UINT8:
                case llvm::memoir::INDEX_WRITE_INT64:
                case llvm::memoir::INDEX_WRITE_INT32:
                case llvm::memoir::INDEX_WRITE_INT16:
                case llvm::memoir::INDEX_WRITE_INT8:
                case llvm::memoir::INDEX_WRITE_DOUBLE:
                case llvm::memoir::INDEX_WRITE_FLOAT:
                case llvm::memoir::INDEX_WRITE_STRUCT_REF:
                case llvm::memoir::INDEX_WRITE_COLLECTION_REF:
                    break;
                case llvm::memoir::ASSOC_WRITE_UINT64:
                case llvm::memoir::ASSOC_WRITE_UINT32:
                case llvm::memoir::ASSOC_WRITE_UINT16:
                case llvm::memoir::ASSOC_WRITE_UINT8:
                case llvm::memoir::ASSOC_WRITE_INT64:
                case llvm::memoir::ASSOC_WRITE_INT32:
                case llvm::memoir::ASSOC_WRITE_INT16:
                case llvm::memoir::ASSOC_WRITE_INT8:
                case llvm::memoir::ASSOC_WRITE_DOUBLE:
                case llvm::memoir::ASSOC_WRITE_FLOAT:
                case llvm::memoir::ASSOC_WRITE_STRUCT_REF:
                case llvm::memoir::ASSOC_WRITE_COLLECTION_REF:
                    assert(false);
                case llvm::memoir::INDEX_GET_STRUCT:
                case llvm::memoir::INDEX_GET_COLLECTION:
                case llvm::memoir::ASSOC_GET_STRUCT:
                case llvm::memoir::ASSOC_GET_COLLECTION:
                case llvm::memoir::DELETE_STRUCT:
                case llvm::memoir::JOIN:
                case llvm::memoir::SLICE:
                case llvm::memoir::ASSERT_STRUCT_TYPE:
                case llvm::memoir::ASSERT_COLLECTION_TYPE:
                case llvm::memoir::SET_RETURN_TYPE:
                case llvm::memoir::DELETE_COLLECTION:
                case llvm::memoir::NONE:
                    break;
            }

        }
    }

}