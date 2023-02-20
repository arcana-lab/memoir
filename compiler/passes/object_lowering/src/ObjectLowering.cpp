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

    llvm::Value* ObjectLowering::FindBasePointerForTensor(
            memoir::Collection* collection_origin,
            std::map<llvm::Value *, llvm::Value *> &replacementMapping,
            std::map<llvm::PHINode*, memoir::ControlPHICollection*> & phiNodesReplacement
            )
    {
        auto collection_kind =  collection_origin->getKind();
        switch(collection_kind)
        {
            case memoir::CollectionKind::BASE: {
                auto base_tensor_origin = static_cast<memoir::BaseCollection*>(collection_origin);
                auto& alloc_ins = base_tensor_origin->getAllocation();
                return replacementMapping.at(&alloc_ins.getCallInst());
            }
            case memoir::CollectionKind::FIELD_ARRAY:
                assert(false && "Field Array Collections shouldn't be visible at this stage");
            case memoir::CollectionKind::NESTED: {
                auto nested_tensor_origin = static_cast<memoir::NestedCollection *>(collection_origin);
                auto &access_ins = nested_tensor_origin->getAccess().getCallInst();
                return replacementMapping.at(&access_ins);
            }
            case memoir::CollectionKind::CONTROL_PHI:{
                auto &ctxt = M.getContext();
                auto control_phi_origin = static_cast<memoir::ControlPHICollection*>(collection_origin);
                auto &original_phi_node = control_phi_origin->getPHI();
                auto i8Ty = llvm::IntegerType::get(ctxt, 8);
                auto i8StarTy = llvm::PointerType::getUnqual(i8Ty);
                IRBuilder<> builder(&original_phi_node);
                auto newPhi = builder.CreatePHI(i8StarTy, original_phi_node.getNumIncomingValues());
                replacementMapping[&original_phi_node] = newPhi;
                phiNodesReplacement[&original_phi_node] = control_phi_origin;
                return newPhi;
            }
            case memoir::CollectionKind::CALL_PHI:
            case memoir::CollectionKind::ARG_PHI:
                assert(false && "TODO when adding interprocedural");
            case memoir::CollectionKind::DEF_PHI: {
                auto def_phi_origin = static_cast<memoir::DefPHICollection *>(collection_origin);
                return FindBasePointerForTensor(&def_phi_origin->getCollection(), replacementMapping,
                                                phiNodesReplacement);
            }
            case memoir::CollectionKind::USE_PHI: {
                auto use_phi_origin = static_cast<memoir::UsePHICollection *>(collection_origin);
                return FindBasePointerForTensor(&use_phi_origin->getCollection(), replacementMapping,
                                                phiNodesReplacement);
            }
            case memoir::CollectionKind::JOIN_PHI:
                assert(false && "Joins should have been lowered down at this point");
        }
    }


//    llvm::GetElementPtrInst

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
                auto nested_struct_origin = static_cast<memoir::ContainedStruct*>(struct_origin);
                auto& parent_struct_get = nested_struct_origin->getAccess();
                return replacementMapping[&parent_struct_get.getCallInst()];
            }
            case memoir::StructCode::REFERENCED: {
                auto referenced_struct_origin = static_cast<memoir::ReferencedStruct*>(struct_origin);
                auto& reference_struct_get =referenced_struct_origin->getAccess();
                return replacementMapping.at(&reference_struct_get.getCallInst());
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
            case memoir::StructCode::ARGUMENT_PHI:
            case memoir::StructCode::RETURN_PHI:
                assert(false && "TODO for later");
        }

    }
    llvm::Value* ObjectLowering::GetGEPForTensorUse( memoir::Collection* collection_origin,
                                                     std::map<llvm::Value *, llvm::Value *> &replacementMapping,
                                                     std::map<llvm::PHINode*, memoir::ControlPHICollection*> & phiNodesReplacement) {

        auto collection_kind = collection_origin->getKind();
        llvm::Value *baseTensorPtr = FindBasePointerForTensor(collection_origin, replacementMapping,
                                                              phiNodesReplacement);
        std::vector<llvm::Value *> indicies;
        memoir::CollectionType *collectionType;
        Instruction* accessIns;
        switch (collection_kind) {
            case memoir::CollectionKind::DEF_PHI: {
                auto def_phi_origin = static_cast<memoir::DefPHICollection *>(collection_origin);
                auto write_tensor = static_cast<memoir::IndexWriteInst *> (&def_phi_origin->getAccess());
                for (unsigned i = 0; i < write_tensor->getNumberOfDimensions(); ++i) {
                    indicies.push_back(&write_tensor->getIndexOfDimension(i));
                }
                collectionType = &def_phi_origin->getType();
                accessIns = &write_tensor->getCallInst();
            }
            case memoir::CollectionKind::USE_PHI: {
                auto use_phi_origin = static_cast<memoir::UsePHICollection *>(collection_origin);
                auto read_tensor = static_cast<memoir::IndexReadInst *> (&use_phi_origin->getAccess());
                for (unsigned i = 0; i < read_tensor->getNumberOfDimensions(); ++i) {
                    indicies.push_back(&read_tensor->getIndexOfDimension(i));
                }
                collectionType = &use_phi_origin->getType();
                accessIns = &read_tensor->getCallInst();
                break;
            }
            case memoir::CollectionKind::NESTED: {
                auto nested_collection_origin = static_cast<memoir::NestedCollection *>(collection_origin);
                auto get_tensor = static_cast<memoir::IndexGetInst *> (&nested_collection_origin->getAccess());
                for (unsigned i = 0; i < get_tensor->getNumberOfDimensions(); ++i) {
                    indicies.push_back(&get_tensor->getIndexOfDimension(i));
                }
                collectionType = &nested_collection_origin->getType();
                accessIns = &get_tensor->getCallInst();
                break;
            }
            default:
                assert(false && "Should be unreachable here");
        }
        auto elem_type = &collectionType->getElementType();
        IRBuilder<> builder(accessIns);
        auto llvm_tensor_elem_type_ptr = PointerType::getUnqual(nativeTypeConverter->getLLVMRepresentation(elem_type));
        if (collectionType->getCode() == memoir::TypeCode::STATIC_TENSOR)
        {
            auto llvm_tensor_type_ptr = PointerType::getUnqual(nativeTypeConverter->getLLVMRepresentation(collectionType));
            auto bitcast = builder.CreateBitCast(baseTensorPtr, llvm_tensor_type_ptr);
            return builder.CreateGEP(llvm_tensor_elem_type_ptr, bitcast, indicies);

        }else if (collectionType->getCode() == memoir::TypeCode::TENSOR)
        {
            auto int64Ty = llvm::Type::getInt64Ty(M.getContext());
            auto tensortype = static_cast<memoir::TensorType*>(collectionType);
            auto ndim  = tensortype->getNumberOfDimensions();
            auto tensor_header_view = builder.CreateBitCast(baseTensorPtr, PointerType::getUnqual(int64Ty));
            std::vector<llvm::Value*> sizes;
            for (unsigned int i = 0; i < ndim; ++i) {
                Value *indexList[1] = {ConstantInt::get(int64Ty, i)};
                auto sizeGEP = builder.CreateGEP(PointerType::getUnqual(int64Ty), tensor_header_view, indexList);
                sizes[i] = builder.CreateLoad(sizeGEP);
            }
            Value *indexList[1] = {ConstantInt::get(int64Ty, ndim)};
            auto skipMetaDataGEP = builder.CreateGEP(PointerType::getUnqual(int64Ty), tensor_header_view,
                                                     indexList);
            auto tensor_body_view  = builder.CreateBitCast(skipMetaDataGEP, llvm_tensor_elem_type_ptr);
            Value *multiCumSizes[ndim];
            multiCumSizes[ndim - 1] = ConstantInt::get(int64Ty, 1);
            Utility::debug() << "Dimension = " << ndim << "\n";
            for (signed long i = ((signed long)ndim) - 2; i >= 0; --i) {
                errs() << "Dimension " << i << " uses" << *multiCumSizes[i + 1] << "and " << *sizes[i + 1] << " \n";
                multiCumSizes[i] = builder.CreateMul(multiCumSizes[i + 1], sizes[i + 1]);
            }
            Value *size = ConstantInt::get(int64Ty, 0);
            for (unsigned int dim = 0; dim < ndim; ++dim) {
                errs() << "Dimension size" << dim << "represented by " <<  *(indicies[dim]) << "\n";
                auto skipsInDim = builder.CreateMul(multiCumSizes[dim], indicies[dim]);
                size = builder.CreateAdd(size, skipsInDim);
            }
            Value *indices[1] = {size};
            return builder.CreateGEP(tensor_body_view, indices);

        }
        else{
            assert(false && "must be a tensor");
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
                case llvm::memoir::INDEX_READ_PTR:
                case llvm::memoir::INDEX_WRITE_PTR:
                case llvm::memoir::STRUCT_WRITE_PTR:
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
                case llvm::memoir::STRUCT_READ_PTR:
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
                case llvm::memoir::STRUCT_READ_COLLECTION_REF:
                case llvm::memoir::STRUCT_GET_STRUCT:
                case llvm::memoir::STRUCT_GET_COLLECTION:{
                    memoir::Struct* struct_accessed;
                    unsigned int field_index;
                    switch(mins->getKind()){
                        case llvm::memoir::STRUCT_GET_STRUCT:
                        case llvm::memoir::STRUCT_GET_COLLECTION:{
                            auto struct_get_ins = static_cast<memoir::StructGetInst *>(mins);
                            struct_accessed = &struct_get_ins->getStructAccessed();
                            field_index = struct_get_ins->getFieldIndex();
                            break;
                        }
                        default:
                        {
                            auto struct_read_ins = static_cast<memoir::StructReadInst *>(mins);
                            struct_accessed = &struct_read_ins->getStructAccessed();
                            field_index = struct_read_ins->getFieldIndex();
                            break;
                        }
                    }
                    auto base_struct_ptr = FindBasePointerForStruct(struct_accessed,
                                                                    replacementMapping,
                                                                    phiNodesReplacement);
                    std::vector<Value*> indices = {llvm::ConstantInt::get(int64Ty, 0), llvm::ConstantInt::get(int64Ty, field_index)};
                    auto gep = builder.CreateGEP(base_struct_ptr,
                                            indices);
                    auto struct_type = (memoir::StructType*) &struct_accessed->getType();
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
                            break;
                        }
                        case memoir::TypeCode::STATIC_TENSOR: {
                            auto static_tensor_type = static_cast<memoir::StaticTensorType *>(&field_type);
                            auto llvm_inner_type = nativeTypeConverter->getLLVMRepresentation((memoir::Type *)
                                                                                                      &static_tensor_type->getElementType());
                            auto bitcast_ins = builder.CreateBitCast(gep, PointerType::getUnqual(llvm_inner_type));
                            replacementMapping[&ins] = bitcast_ins;
                            break;
                        }
                        case memoir::TypeCode::REFERENCE: {
                            auto reference_type = static_cast<memoir::StaticTensorType *>(&field_type );
                            auto elem_type = (memoir::Type *) &reference_type->getElementType();
                            auto elem_type_llvm = nativeTypeConverter->getLLVMRepresentation(elem_type);
                            auto loadInst =
                                    builder.CreateLoad(llvm::PointerType::getUnqual(elem_type_llvm), gep, "baseload");
                            replacementMapping[&ins] = loadInst;
                            break;
                        }
                        case memoir::TypeCode::STRUCT: {
                            auto inner_struct_type = (memoir::Type *) (&field_type);
                            auto llvm_struct_type = nativeTypeConverter->getLLVMRepresentation(inner_struct_type);
                            auto bitcast = builder.CreateBitCast(gep, PointerType::getUnqual(llvm_struct_type));
                            replacementMapping[&ins] = bitcast;
                            break;
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
                case llvm::memoir::STRUCT_WRITE_COLLECTION_REF: {
                    auto struct_read_ins = static_cast<memoir::StructWriteInst *>(mins);
                    auto struct_accessed = &struct_read_ins->getStructAccessed();
                    auto field_index = struct_read_ins->getFieldIndex();
                    auto base_struct_ptr = FindBasePointerForStruct(struct_accessed,
                                                                    replacementMapping,
                                                                    phiNodesReplacement);
                    Value *indices[2] = {llvm::ConstantInt::get(int64Ty, 0),
                                         llvm::ConstantInt::get(int64Ty, field_index)};
                    auto gep = builder.CreateGEP(base_struct_ptr,
                                                 indices);
                    auto struct_type = (memoir::StructType *) &struct_accessed->getType();
                    auto &field_type = struct_type->getFieldType(field_index);
                    Value* reference_value = field_type.getCode() == memoir::TypeCode::REFERENCE ?
                            replacementMapping[&struct_read_ins->getValueWritten()] :
                                             &struct_read_ins->getValueWritten();
                    auto storeInst = builder.CreateStore(reference_value, gep);
                    replacementMapping[&ins] = storeInst;
                    break;
                }
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
                case llvm::memoir::INDEX_GET_STRUCT:
                case llvm::memoir::INDEX_GET_COLLECTION:

                    break;
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
                case llvm::memoir::INDEX_WRITE_COLLECTION_REF: {
                    auto tensor_write_ins = static_cast<memoir::IndexWriteInst *>(mins);
//                    tensor_write_ins->getCollectionAccessed()
                    break;
                }
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
                case llvm::memoir::ASSOC_READ_PTR:
                    assert(false&& "Assoc. array should have been lowered down already");

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
                case llvm::memoir::ASSOC_WRITE_PTR:
                case llvm::memoir::ASSOC_GET_STRUCT:
                case llvm::memoir::ASSOC_GET_COLLECTION:
                    assert(false&& "Assoc. array should have been lowered down already");
                case llvm::memoir::DELETE_STRUCT:
                    Utility::debug()<< "DELETE_STRUCT not yet implemented. TODO \n";
                    break;
                case llvm::memoir::JOIN:
                case llvm::memoir::SLICE:
                    assert(false&& "sequences should have been lowered down already");
                case llvm::memoir::DELETE_COLLECTION:
                    Utility::debug()<< "DELETE_STRUCT not yet implemented. TODO \n";
                    break;
                case llvm::memoir::ASSERT_STRUCT_TYPE:
                case llvm::memoir::ASSERT_COLLECTION_TYPE:
                case llvm::memoir::SET_RETURN_TYPE:
                    Utility::debug()<< "TODO Interprocedural \n";
                    break;
                case llvm::memoir::NONE:
                    break;
            }

        }
    }

}