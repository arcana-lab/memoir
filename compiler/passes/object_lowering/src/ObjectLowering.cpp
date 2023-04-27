#include "ObjectLowering.hpp"
#include <functional>
#include "memoir/ir/Instructions.hpp"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "Utility.h"
#include "memoir/utility/Metadata.hpp"
#include "memoir/ir/Function.hpp"
#include "memoir/ir/Types.hpp"
#include "memoir/analysis/TypeAnalysis.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/analysis/StructAnalysis.hpp"

using namespace llvm::memoir;

namespace object_lowering {
    ObjectLowering::ObjectLowering(llvm::Module &M, llvm::ModulePass *mp,Noelle& n) : M(M), mp(mp) {
        nativeTypeConverter = new NativeTypeConverter(M);
        auto &CA = CollectionAnalysis::get(n);
    }


    void findInstsToDelete(Value *i, std::set<Value *> &toDelete) {
        for (auto u: i->users()) {
            if (toDelete.find(u) != toDelete.end())
                continue;
            toDelete.insert(u);
            findInstsToDelete(u, toDelete);
        }
    }

    bool function_has_memoir(llvm::Function* f)
    {
        if((!f->hasName()) || f->isDeclaration())
        {
            return false;
        }

        for (auto& i :instructions(f) )
        {
            auto m = memoir::MemOIRInst::get(i);
            if(m)
            {
                auto memoirkind = m->getKind();
                switch(memoirkind)
                {
                    case llvm::memoir::SIZE:
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
                    default:
                        return true;
                }
            }
        }
        return false;
    }

    void test(llvm::Module &M)
    {
        Function* main;
        for (auto &f: M) {
            if (f.hasName() && f.getName() == "main") {
               main = &f;
               break;
            }
        }
        for(auto& ins_ref : instructions(*main))
        {
            auto m = memoir::MemOIRInst::get(ins_ref);
            if(m == nullptr)
            {
                continue;
            }
            auto mins = dyn_cast<memoir::IndexReadInst>(m);
            if(mins == nullptr)
            {
                continue;
            }
            errs() << ins_ref << "\n";
            auto tensortype = dyn_cast<memoir::TensorType>(&mins->getCollectionType());
            assert(tensortype && "it shouldnt be null lol");
            errs() << "the tensor type has "<<tensortype->getNumberOfDimensions() << "dimensions\n";
        }
    }

    void ObjectLowering::transform() {
        std::vector<Function *> functions_to_clone;
        llvm::Function* main;
        for (auto &f: M) {
            if (function_has_memoir(&f)) {
                if(f.getName() == "main")
                {
                    main = &f;
                }
                else {
                    functions_to_clone.push_back(&f);
                }
            }
        }
        auto& typeAnalysis = memoir::TypeAnalysis::get();
        for (auto &oldF: functions_to_clone) {
            Utility::debug()<<"Attempting to clone function "<< oldF->getName() << "\n";
            if (oldF->getFunctionType()->isVarArg()) {
                assert(false && "var arg function not supported");
            }
            std::vector<llvm::Type *> arg_types;
            for (unsigned argi = 0; argi < oldF->arg_size(); ++argi) {
                auto old_arg = (oldF->arg_begin() + argi);
                memoir::Type *marg_ty = typeAnalysis.getType(*old_arg);
                arg_types.push_back(marg_ty != nullptr ?
                                    llvm::PointerType::getUnqual(nativeTypeConverter->getLLVMRepresentation(marg_ty)):
                                    old_arg->getType());
            }
            memoir::Type *mret_ty = typeAnalysis.getReturnType(*oldF);
            llvm::Type *ret_ty = mret_ty != nullptr ?
                                 llvm::PointerType::getUnqual(nativeTypeConverter->getLLVMRepresentation(mret_ty)):
                                 oldF->getReturnType();
            auto new_func_ty = FunctionType::get(ret_ty, arg_types, false);
            auto newF = Function::Create(new_func_ty,
                                         oldF->getLinkage(),
                                         oldF->getAddressSpace(),
                                         oldF->getName()+"cloned-lowered",
                                         oldF->getParent());
            newF->getBasicBlockList().splice(newF->begin(), oldF->getBasicBlockList());
            for (unsigned argi = 0; argi < oldF->arg_size(); ++argi) {
                auto old_arg = (oldF->arg_begin() + argi);
                auto new_arg = (newF->arg_begin() + argi);
                memoir::Type *marg_ty = typeAnalysis.getType(*old_arg);
                if(marg_ty != nullptr){
                    replacementMapping[old_arg] = new_arg;
                }
                else{
                    old_arg->replaceAllUsesWith(new_arg);
                }
            }
            TypeAnalysis::invalidate();
            CollectionAnalysis::invalidate();
            StructAnalysis::invalidate();
            Utility::debug()<<"Cloned function "<< *newF << "\n";
            clonedFunctionMap[oldF] = newF;

        }

        function_transform(main);

        for (auto & func_pair: clonedFunctionMap)
        {
            function_transform(func_pair.second);
        }

        for (auto & func_pair: clonedFunctionMap)
        {
            func_pair.first->eraseFromParent();
        }
        for (auto v: toDeletes) {
            // errs() << "\t" << *v << "\n";
            if (auto i = dyn_cast<Instruction>(v)) {
                i->replaceAllUsesWith(UndefValue::get(i->getType()));
                i->eraseFromParent();
            }
        }

        Utility::debug()<<"main(after) function "<<  *main <<"\n\n";
        for (auto & func_pair: clonedFunctionMap)
        {
            auto f = func_pair.first;
            Utility::debug()<<"Cloned (after)function "<< *func_pair.second << "\n\n";
        }
    }

    void ObjectLowering::function_transform(llvm::Function *f) {
        Utility::debug() << "\n Starting transformation on " << f->getName() << "\n\n";
        //

        std::set<memoir::ControlPHIStruct *> phiNodesReplacementStruct;
        std::set<memoir::ControlPHICollection *> phiNodesReplacementCollection;

        DominatorTree &DT =
                mp->getAnalysis<DominatorTreeWrapperPass>(*f).getDomTree();
        auto &entry = f->getEntryBlock();

        BasicBlockTransformer(DT,
                              &entry,
                              phiNodesReplacementStruct,
                              phiNodesReplacementCollection);
        std::set<llvm::PHINode *> old_phi_nodes;
        for (auto control_phi_struct: phiNodesReplacementStruct) {
            old_phi_nodes.insert(&control_phi_struct->getPHI());
        }
        for (auto control_phi_collection: phiNodesReplacementCollection) {
            old_phi_nodes.insert(&control_phi_collection->getPHI());
        }

        for (auto old_phi: old_phi_nodes) {
//            auto llvm
            auto new_phi = dyn_cast<PHINode>(replacementMapping.at(old_phi));
            for (unsigned int i = 0; i < old_phi->getNumIncomingValues(); i++) {
                auto old_val = old_phi->getIncomingValue(i);
                if (dyn_cast<ConstantPointerNull>(old_val)) {
                    auto new_val =
                            ConstantPointerNull::get(dyn_cast<llvm::PointerType>(new_phi->getType()));
                    new_phi->addIncoming(new_val, old_phi->getIncomingBlock(i));
                } else {
                    auto new_val = replacementMapping.at(old_val);
                    new_phi->addIncoming(new_val, old_phi->getIncomingBlock(i));
                }
            }
        }
        for (auto p: replacementMapping) {
            toDeletes.insert(p.first);
        }

    }

    llvm::Value *ObjectLowering::FindBasePointerForTensor(
            memoir::Collection *collection_origin,
            std::set<memoir::ControlPHICollection *> &phiNodesReplacement
    ) {
        auto collection_kind = collection_origin->getKind();
        switch (collection_kind) {
            case memoir::CollectionKind::SLICE:
                assert(false && "slices are unsupported");
            case memoir::CollectionKind::REFERENCED: {
                auto referenced_tensor_origin = static_cast<memoir::ReferencedCollection *>(collection_origin);
                auto &access_ins = referenced_tensor_origin->getAccess().getCallInst();
                return replacementMapping.at(&access_ins);
            }
            case memoir::CollectionKind::BASE: {
                auto base_tensor_origin = static_cast<memoir::BaseCollection *>(collection_origin);
                auto &alloc_ins = base_tensor_origin->getAllocation();
                return replacementMapping.at(&alloc_ins.getCallInst());
            }
            case memoir::CollectionKind::FIELD_ARRAY:
                assert(false && "Field Array Collections shouldn't be visible at this stage");
            case memoir::CollectionKind::NESTED: {
                auto nested_tensor_origin = static_cast<memoir::NestedCollection *>(collection_origin);
                auto &access_ins = nested_tensor_origin->getAccess().getCallInst();
                return replacementMapping.at(&access_ins);
            }
            case memoir::CollectionKind::CONTROL_PHI: {
                auto &ctxt = M.getContext();
                auto control_phi_origin = static_cast<memoir::ControlPHICollection *>(collection_origin);
                auto &original_phi_node = control_phi_origin->getPHI();
                auto i8Ty = llvm::IntegerType::get(ctxt, 8);
                auto i8StarTy = llvm::PointerType::getUnqual(i8Ty);
                IRBuilder<> builder(&original_phi_node);
                auto newPhi = builder.CreatePHI(i8StarTy, original_phi_node.getNumIncomingValues());
                replacementMapping[&original_phi_node] = newPhi;
                phiNodesReplacement.insert(control_phi_origin);
                return newPhi;
            }
            case memoir::CollectionKind::RET_PHI:{
                auto ret_phi_origin = static_cast<memoir::RetPHICollection *>(collection_origin);
                return replacementMapping.at(&ret_phi_origin->getCall());
            }
            case memoir::CollectionKind::ARG_PHI:{
                auto ret_phi_origin = static_cast<memoir::ArgPHICollection *>(collection_origin);
                return replacementMapping.at(&ret_phi_origin->getArgument());
            }
            case memoir::CollectionKind::DEF_PHI: {
                assert(false && "Lowering shouldn't enoucnter def phi");
            }
            case memoir::CollectionKind::USE_PHI: {
                assert(false && "Lowering shouldn't encounter use phi");
            }
            case memoir::CollectionKind::JOIN_PHI:
                assert(false && "Joins should have been lowered down at this point");
        }
    }


//    llvm::GetElementPtrInst

    llvm::Value *ObjectLowering::FindBasePointerForStruct(
            memoir::Struct *struct_origin,
            std::set<memoir::ControlPHIStruct *> &phiNodesReplacement) {
        auto struct_code = struct_origin->getCode();
        switch (struct_code) {
            case memoir::StructCode::BASE: {
                auto base_struct_origin = static_cast<memoir::BaseStruct *>(struct_origin);
                auto &allocInst = base_struct_origin->getAllocInst();
                return replacementMapping.at(&allocInst.getCallInst());
            }
            case memoir::StructCode::CONTAINED:
            case memoir::StructCode::NESTED: {
                auto nested_struct_origin = static_cast<memoir::ContainedStruct *>(struct_origin);
                auto &parent_struct_get = nested_struct_origin->getAccess();
                return replacementMapping.at(&parent_struct_get.getCallInst());
            }
            case memoir::StructCode::REFERENCED: {
                auto referenced_struct_origin = static_cast<memoir::ReferencedStruct *>(struct_origin);
                auto &reference_struct_get = referenced_struct_origin->getAccess();
                return replacementMapping.at(&reference_struct_get.getCallInst());
            }
            case memoir::StructCode::CONTROL_PHI: {
                auto control_phi_origin = static_cast<memoir::ControlPHIStruct *>(struct_origin);
                auto &original_phi_node = control_phi_origin->getPHI();
                auto struct_type = (memoir::StructType *) &control_phi_origin->getType();
                auto struct_type_as_type = static_cast<memoir::Type *>(struct_type);
                auto llvmType = nativeTypeConverter->getLLVMRepresentation(struct_type_as_type);
                auto llvmPtrType = llvm::PointerType::getUnqual(llvmType);
                IRBuilder<> builder(&original_phi_node);
                auto newPhi = builder.CreatePHI(llvmPtrType, original_phi_node.getNumIncomingValues());
                replacementMapping[&original_phi_node] = newPhi;
                phiNodesReplacement.insert(control_phi_origin);
                return newPhi;
            }
            case memoir::StructCode::ARGUMENT_PHI: {
                auto argument_phi_origin = static_cast<memoir::ArgPHIStruct *>(struct_origin);
                return replacementMapping.at(&argument_phi_origin->getArgument());
            }
            case memoir::StructCode::RETURN_PHI:{
                auto return_phi_origin = static_cast<memoir::RetPHIStruct *>(struct_origin);
                return  replacementMapping.at(&return_phi_origin->getCall());
            }

        }

    }

    llvm::Value *ObjectLowering::GetGEPForTensorUse(memoir::MemOIRInst *access_ins,
                                                    std::set<memoir::ControlPHICollection *> &phiNodesReplacement) {

        std::vector<llvm::Value *> indicies;
        Instruction *accessIns;
        memoir::Collection *collection_origin;
        if (memoir::IndexWriteInst::classof(access_ins)) {
            auto write_tensor = static_cast<memoir::IndexWriteInst *> (access_ins);
            for (unsigned i = 0; i < write_tensor->getNumberOfDimensions(); ++i) {
                indicies.push_back(&write_tensor->getIndexOfDimension(i));
            }
            collection_origin = &write_tensor->getCollectionAccessed();
            accessIns = &write_tensor->getCallInst();
        } else if (memoir::IndexReadInst::classof(access_ins)) {
            auto read_tensor = static_cast<memoir::IndexReadInst *> (access_ins);
            for (unsigned i = 0; i < read_tensor->getNumberOfDimensions(); ++i) {
                indicies.push_back(&read_tensor->getIndexOfDimension(i));
            }
            collection_origin = &read_tensor->getCollectionAccessed();
            accessIns = &read_tensor->getCallInst();
        } else if (memoir::IndexGetInst::classof(access_ins)) {
            auto get_tensor = static_cast<memoir::IndexGetInst *> (access_ins);
            for (unsigned i = 0; i < get_tensor->getNumberOfDimensions(); ++i) {
                indicies.push_back(&get_tensor->getIndexOfDimension(i));
            }
            collection_origin = &get_tensor->getCollectionAccessed();
            accessIns = &get_tensor->getCallInst();
        } else {
            assert(false && "instruction for this function must be a tensor operate memoir func");
        }

        auto collectionType = &collection_origin->getType();

        llvm::Value *baseTensorPtr = FindBasePointerForTensor(collection_origin,
                                                              phiNodesReplacement);
        auto elem_type = &collectionType->getElementType();
        IRBuilder<> builder(accessIns);
        auto llvm_tensor_elem_type_ptr = llvm::PointerType::getUnqual(nativeTypeConverter->getLLVMRepresentation(elem_type));
        if (collectionType->getCode() == memoir::TypeCode::STATIC_TENSOR) {
            auto llvm_tensor_type_ptr = llvm::PointerType::getUnqual(
                    nativeTypeConverter->getLLVMRepresentation(collectionType));
            auto bitcast = builder.CreateBitCast(baseTensorPtr, llvm_tensor_type_ptr);
            return builder.CreateGEP(llvm_tensor_elem_type_ptr, bitcast, indicies);

        } else if (collectionType->getCode() == memoir::TypeCode::TENSOR) {
            auto int64Ty = llvm::Type::getInt64Ty(M.getContext());
            auto int32Ty = llvm::Type::getInt32Ty(M.getContext());
            auto tensortype = static_cast<memoir::TensorType *>(collectionType);
            auto ndim = tensortype->getNumberOfDimensions();
            auto tensor_header_view = builder.CreateBitCast(baseTensorPtr, llvm::PointerType::getUnqual(int64Ty));
            std::vector<llvm::Value *> sizes;
            for (unsigned int i = 0; i < ndim; ++i) {
                Value *indexList[1] = {ConstantInt::get(int32Ty, i)};
                auto sizeGEP = builder.CreateGEP(tensor_header_view, indexList);
                sizes[i] = builder.CreateLoad(sizeGEP);
                Utility::debug() << "index "<< i << "will be loaded with " << *sizeGEP << "\n" << sizes[i] << "\n";
            }
            Value *indexList[1] = {ConstantInt::get(int32Ty, ndim)};
            auto skipMetaDataGEP = builder.CreateGEP(tensor_header_view,indexList);
            Utility::debug() << "skip Header Gep is " << *skipMetaDataGEP <<"\n";
            auto tensor_body_view = builder.CreateBitCast(skipMetaDataGEP, llvm_tensor_elem_type_ptr);
            Utility::debug() << "Bitcasted to  " << *tensor_body_view <<"\n";
            Value *multiCumSizes[ndim];
            multiCumSizes[ndim - 1] = ConstantInt::get(int32Ty, 1);
            Utility::debug() << "num dimensions = " << ndim << "\n";
            for (signed long i = ((signed long) ndim) - 2; i >= 0; --i) {
                errs() << "Dimension " << i << " uses" << *multiCumSizes[i + 1] << "and " << *sizes[i + 1] << " \n";
                multiCumSizes[i] = builder.CreateMul(multiCumSizes[i + 1], sizes[i + 1]);
            }
            Value *size = ConstantInt::get(int32Ty, 0);
            for (unsigned int dim = 0; dim < ndim; ++dim) {
                errs() << "Dimension size" << dim << "represented by " << *(indicies[dim]) << "\n";
                auto skipsInDim = builder.CreateMul(multiCumSizes[dim], indicies[dim]);
                size = builder.CreateAdd(size, skipsInDim);
            }
            Value *indices[1] = {size};
            return builder.CreateGEP(tensor_body_view, indices);

        } else {
            assert(false && "must be a tensor");
        }

    }

    void ObjectLowering::BasicBlockTransformer(llvm::DominatorTree &DT,
                                               llvm::BasicBlock *bb,
                                               std::set<memoir::ControlPHIStruct *> &phiNodesReplacementStruct,
                                               std::set<memoir::ControlPHICollection *> &phiNodesReplacementCollection) {
        auto &ctxt = M.getContext();
        auto int64Ty = llvm::Type::getInt64Ty(ctxt);
        auto int32Ty = llvm::Type::getInt32Ty(ctxt);
        auto i8Ty = llvm::IntegerType::get(ctxt, 8);
        auto i8StarTy = llvm::PointerType::getUnqual(i8Ty);
        auto int64StarTy = llvm::PointerType::getUnqual(int64Ty);
        auto voidTy = llvm::Type::getVoidTy(ctxt);
        auto mallocFTY =
                llvm::FunctionType::get(i8StarTy, ArrayRef<llvm::Type *>({int64Ty}),
                                        false);
        auto mallocf = M.getOrInsertFunction("malloc", mallocFTY);
//        auto freeFTY =
//                llvm::FunctionType::get(voidTy, ArrayRef<Type *>({i8StarTy}),
//                                        false);

//        auto freef = M.getOrInsertFunction("free", freeFTY);

        auto& typeAnalysis = memoir::TypeAnalysis::get();
        for (auto &ins: *bb) {
            Utility::debug() << "Processing Instruction " << ins << "\n";
            IRBuilder<> builder(&ins);
            auto mins = memoir::MemOIRInst::get(ins);
            if (mins == nullptr) {
                Utility::debug() << "Instruction is not a memoir instruction\n";
                if (auto callIns = dyn_cast<CallInst>(&ins)) {
                    auto callee = callIns->getCalledFunction();
                    if (clonedFunctionMap.find(callee) != clonedFunctionMap.end()) {
                        // this is a function call which passes or returns Object*s
                        // replace the arguments
                        llvm::Function* oldF = clonedFunctionMap.at(callee);
                        std::vector<Value *> arguments;
                        for (auto &arg: callIns->args()) {
                            auto val = arg.get();
                            if (replacementMapping.find(val) != replacementMapping.end()) {
                                arguments.push_back(replacementMapping[val]);
                            } else {
                                arguments.push_back(val);
                            }
                        }
                        auto new_callins =
                                builder.CreateCall(clonedFunctionMap[callee], arguments);
                        if(typeAnalysis.getReturnType(*oldF) == nullptr) {
                            ins.replaceAllUsesWith(new_callins);
                        }
                        Utility::debug() << "Call instruction replaced with " <<*new_callins<<"\n";
                         replacementMapping[callIns] = new_callins;
                    }
                }
                else if (auto bitcast = dyn_cast<BitCastInst>(&ins)) {
                    auto casted = bitcast->getOperand(0);
                    if (replacementMapping.find(casted) != replacementMapping.end()) {
                        replacementMapping[bitcast] = replacementMapping[casted];
                        Utility::debug() << *bitcast << "now is linked to the same entry as " << *casted << "which is "
                               << *(replacementMapping[casted]) << "\n";
                    }
                }
                else if (auto icmp = dyn_cast<ICmpInst>(&ins)) {
                    Value *not_the_null_one =
                            dyn_cast<ConstantPointerNull>(icmp->getOperand(0))
                            ? icmp->getOperand(1)
                            : icmp->getOperand(0);
                    if (replacementMapping.find(not_the_null_one)
                        == replacementMapping.end()) {
                        continue;
                    }
                    auto newType = replacementMapping[not_the_null_one]->getType();
                    assert(isa<llvm::PointerType>(newType));
                    auto pointerNewType = dyn_cast<llvm::PointerType>(newType);
                    Value *new_left;
                    auto curLeft = icmp->getOperand(0);
                    if (isa<ConstantPointerNull>(curLeft)) {
                        new_left = ConstantPointerNull::get(pointerNewType);
                    } else if (replacementMapping.find(curLeft) ==
                               replacementMapping.end()) {
                        errs() << "can't find " << *curLeft << "in replacement mapping";
                        assert(false);
                    } else {
                        new_left = replacementMapping[curLeft];
                    }
                    Value *new_right;
                    auto curRight = icmp->getOperand(1);
                    if (isa<ConstantPointerNull>(curRight)) {
                        new_right = ConstantPointerNull::get(pointerNewType);
                    } else if (replacementMapping.find(curRight)
                               == replacementMapping.end()) {
                        errs() << "can't find " << *curRight << "in replacement mapping";
                        assert(false);
                    } else {
                        new_right = replacementMapping[curRight];
                    }
                    errs() << "the left operand is " << *new_left << "\n";
                    errs() << "the right operand is " << *new_right << "\n";
                    auto newIcmp =
                            builder.CreateICmp(icmp->getPredicate(), new_left, new_right);
                    replacementMapping[icmp] = newIcmp;
                    icmp->replaceAllUsesWith(newIcmp);
                }
                else if (auto retIns = dyn_cast<ReturnInst>(&ins)) {
                    // replace returned value, if necessary
                    auto r_val = retIns->getReturnValue();
                    if (replacementMapping.find(r_val) != replacementMapping.end()) {
                        auto new_ret = builder.CreateRet(replacementMapping[r_val]);
                        replacementMapping[retIns] = new_ret;
                    }
                }
                continue;
            }
            switch (mins->getKind()) {
                case llvm::memoir::SIZE:
                    assert(false);
                    break;
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
                    auto struct_type = static_cast<memoir::StructType *>(&type);
                    auto llvmType = nativeTypeConverter->getLLVMRepresentation(&type);
                    auto llvmTypeSize = llvm::ConstantInt::get(
                            int64Ty,
                            M.getDataLayout().getTypeAllocSize(llvmType));
                    std::vector<Value *> arguments{llvmTypeSize};
                    auto newMallocCall = builder.CreateCall(mallocf, arguments, "alloc"+struct_type->getName());
                    Utility::debug() << "Allocate Struct created malloc call : " << *newMallocCall << "\n";
                    auto bc_inst =
                            builder.CreateBitCast(newMallocCall,
                                                  llvm::PointerType::getUnqual(llvmType),
                                                  "allocbitcast"+struct_type->getName());
                    Utility::debug() << "Allocate Struct bitcast call : " << *bc_inst << "\n";
                    replacementMapping[&ins] = bc_inst;
                    break;
                }
                case llvm::memoir::ALLOCATE_TENSOR: {
                    auto alloc_tensor_ins = static_cast<memoir::TensorAllocInst *>(mins);
                    auto &eletype = alloc_tensor_ins->getElementType();
                    auto *llvmType = nativeTypeConverter->getLLVMRepresentation(&eletype);
                    auto llvmTypeSize = llvm::ConstantInt::get(
                            int64Ty,
                            M.getDataLayout().getTypeAllocSize(llvmType));
                    Utility::debug() << "Tensor Element has Size  : " << *llvmTypeSize << "\n";
                    Value *finalSize;
                    finalSize = llvmTypeSize;
                    auto numdim = alloc_tensor_ins->getNumberOfDimensions();
                    for (unsigned long long i = 0; i < numdim; ++i) {
                        finalSize = builder.CreateMul(finalSize, &alloc_tensor_ins->getLengthOfDimensionOperand(i));
                        Utility::debug() << "Size instruction so far for " << i << " :"<< *finalSize << "\n";
                    }
                    auto int64Size = llvm::ConstantInt::get(int64Ty,
                                                            M.getDataLayout().getTypeAllocSize(int64Ty));

                    auto headerSize = builder.CreateMul(llvm::ConstantInt::get(int64Ty, numdim),
                                                        int64Size);
                    Utility::debug() << "Headersize instruction:" << *headerSize << "\n";
                    finalSize = builder.CreateAdd(finalSize, headerSize);
                    std::vector<Value *> arguments{finalSize};
                    auto newMallocCall = builder.CreateCall(mallocf, arguments);
                    Utility::debug() << "Malloc for tensor:" << *newMallocCall << "\n";
                    auto bcInst = builder.CreateBitCast(newMallocCall, int64StarTy);
                    Utility::debug() << "Bitcast :" << *bcInst << "\n";
                    replacementMapping[&ins] = bcInst;
                    for (unsigned long long i = 0; i < numdim; ++i) {
                        std::vector<Value *> indexList{ConstantInt::get(int64Ty, i)};
                        auto gep = builder.CreateGEP(bcInst, indexList);
                        Utility::debug() << "GEP for Dimension" << i << " :" <<*gep << "\n";
                        auto storeins =  builder.CreateStore(&alloc_tensor_ins->getLengthOfDimensionOperand(i), gep);
                        Utility::debug() << "And the stores for it" << *storeins << "\n";
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
                case llvm::memoir::STRUCT_GET_COLLECTION: {
                    Utility::debug() << "Entering Struct Read/Get \n";
                    memoir::Struct *struct_accessed;
                    unsigned int field_index;
                    switch (mins->getKind()) {
                        case llvm::memoir::STRUCT_GET_STRUCT:
                        case llvm::memoir::STRUCT_GET_COLLECTION: {
                            auto struct_get_ins = static_cast<memoir::StructGetInst *>(mins);
                            struct_accessed = &struct_get_ins->getStructAccessed();
                            field_index = struct_get_ins->getFieldIndex();
                            break;
                        }
                        default: {
                            auto struct_read_ins = static_cast<memoir::StructReadInst *>(mins);
                            struct_accessed = &struct_read_ins->getStructAccessed();
                            field_index = struct_read_ins->getFieldIndex();
                            break;
                        }
                    }

                    Utility::debug() << "Struct Read/Get is getting index: " << field_index << "\n";
                    auto base_struct_ptr = FindBasePointerForStruct(struct_accessed,
                                                                    phiNodesReplacementStruct);
                    Utility::debug() << "Struct Read/Get has base pointer being: " << *base_struct_ptr << "\n";
                    auto struct_type = (memoir::StructType *) &struct_accessed->getType();
                    std::vector<Value *> indices = {llvm::ConstantInt::get(int32Ty, 0),
                                                    llvm::ConstantInt::get(int32Ty, field_index)};
                    auto gep = builder.CreateGEP(base_struct_ptr,
                                                 indices, "structreadget"+struct_type->getName());
                    Utility::debug() << "Struct Read/Get created the GEP: " << *gep << "\n";
                    auto &field_type = struct_type->getFieldType(field_index);

                    switch (field_type.getCode()) {
                        case memoir::TypeCode::INTEGER:
                        case memoir::TypeCode::FLOAT:
                        case memoir::TypeCode::DOUBLE:
                        case memoir::TypeCode::POINTER: {
                            auto targetType = nativeTypeConverter->getLLVMRepresentation(&field_type);
                            auto loadInst =
                                    builder.CreateLoad(targetType, gep, "loadbase");
                            replacementMapping[&ins] = loadInst;
                            Utility::debug() << "Emitted " <<*loadInst << "\n";
                            ins.replaceAllUsesWith(loadInst);
                            break;
                        }
                        case memoir::TypeCode::STATIC_TENSOR: {
                            auto static_tensor_type = static_cast<memoir::StaticTensorType *>(&field_type);
                            auto llvm_inner_type = nativeTypeConverter->getLLVMRepresentation((memoir::Type *)
                                                                                                      &static_tensor_type->getElementType());
                            auto bitcast_ins = builder.CreateBitCast(gep, llvm::PointerType::getUnqual(llvm_inner_type));
                            replacementMapping[&ins] = bitcast_ins;
                            Utility::debug() << "Emitted " <<*bitcast_ins << "\n";
                            break;
                        }
                        case memoir::TypeCode::REFERENCE: {
                            auto referenced_type_llvm = nativeTypeConverter->getLLVMRepresentation(&field_type);
                            auto loadInst =
                                    builder.CreateLoad(referenced_type_llvm, gep, "loadref");
//                            Utility::debug() << "Emitted " <<*loadInst << "\n";
//                            auto  bitcast_ins = builder.CreateBitCast(loadInst,referenced_type_llvm);
                            replacementMapping[&ins] = loadInst;
                            Utility::debug() << "Emitted " <<*loadInst << "\n";
                            break;
                        }
                        case memoir::TypeCode::STRUCT: {
                            auto inner_struct_type = (memoir::Type *) (&field_type);
                            auto llvm_struct_type = nativeTypeConverter->getLLVMRepresentation(inner_struct_type);
                            auto bitcast = builder.CreateBitCast(gep, llvm::PointerType::getUnqual(llvm_struct_type));
                            replacementMapping[&ins] = bitcast;
                            Utility::debug() << "Emitted " <<*bitcast << "\n";
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
                case llvm::memoir::STRUCT_WRITE_PTR:
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
                    Utility::debug() << "Entering Struct Write \n";
                    auto struct_write_ins = static_cast<memoir::StructWriteInst *>(mins);
                    auto struct_accessed = &struct_write_ins->getStructAccessed();
                    Utility::debug() << "Found the struct  that is it's attempting to access for write  \n";
                    auto field_index = struct_write_ins->getFieldIndex();
                    auto base_struct_ptr = FindBasePointerForStruct(struct_accessed,
                                                                    phiNodesReplacementStruct);

                    Utility::debug() << "Found the base struct ptr for the write \n";
                    auto structType = struct_accessed->getType();
                    Utility::debug() << "The struct type for this write has " << structType.getNumFields() << "fields\n";
                    auto struct_name = structType.getName();
                    Utility::debug() << "Struct has name "<< struct_name<< "\n";
                    std::vector<llvm::Value *> indices = {llvm::ConstantInt::get(int32Ty, 0),
                                                          llvm::ConstantInt::get(int32Ty, field_index)};
                    Utility::debug() << "Struct Write has base struct pointer as : " << *base_struct_ptr << "\n";
                    Utility::debug() << "and the field it's indexing is " << field_index <<"\n";
                    auto gep = builder.CreateGEP(base_struct_ptr,
                                                 indices, "structwritegep"+struct_name);
                    Utility::debug() << "Struct Write created the GEP: " << *gep << "\n";
                    auto struct_type = (memoir::StructType *) &struct_accessed->getType();
                    auto &field_type = struct_type->getFieldType(field_index);
                    Value *reference_value = field_type.getCode() == memoir::TypeCode::REFERENCE ?
                                             replacementMapping.at(&struct_write_ins->getValueWritten()) :
                                             &struct_write_ins->getValueWritten();
                    auto storeInst = builder.CreateStore(reference_value, gep);
                    Utility::debug() << "Struct Write created the store: " << *storeInst << "\n";
                    replacementMapping[&ins] = storeInst;
                    Utility::debug() << "Emitted " <<*storeInst << "\n";
                    break;
                }


                case llvm::memoir::INDEX_READ_PTR:
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
                case llvm::memoir::INDEX_READ_COLLECTION_REF: {
                    auto gep = GetGEPForTensorUse(mins, phiNodesReplacementCollection);
                    auto index_read_ins = static_cast<memoir::IndexReadInst *>(mins);
                    auto elem_type = &index_read_ins->getCollectionAccessed().getType().getElementType();
                    auto llvm_elem_type = nativeTypeConverter->getLLVMRepresentation(elem_type);
                    switch (mins->getKind()) {
                        case llvm::memoir::INDEX_READ_STRUCT_REF:
                        case llvm::memoir::INDEX_READ_COLLECTION_REF: {
                            auto loadInst = builder.CreateLoad(i8StarTy, gep, "refload");
                            auto bc_inst = builder.CreateBitCast(loadInst, llvm::PointerType::getUnqual(llvm_elem_type));
                            replacementMapping[&ins] = bc_inst;
                            break;
                        }
                        default: {
                            auto loadInst = builder.CreateLoad(llvm_elem_type, gep, "baseload");
                            replacementMapping[&ins] = loadInst;
                            ins.replaceAllUsesWith(loadInst);
                            break;
                        }
                    }
                    break;
                }
                case llvm::memoir::INDEX_GET_STRUCT:
                case llvm::memoir::INDEX_GET_COLLECTION: {
                    auto gep = GetGEPForTensorUse(mins, phiNodesReplacementCollection);
                    replacementMapping[&ins] = gep;
                    break;
                }
                case llvm::memoir::INDEX_WRITE_PTR:
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
                    auto gep = GetGEPForTensorUse(mins, phiNodesReplacementCollection);
                    auto index_write_ins = static_cast<memoir::IndexWriteInst *>(mins);
//                    auto elem_type = &index_write_ins->getCollectionAccessed().getType().getElementType();
//                    auto llvm_elem_type = nativeTypeConverter->getLLVMRepresentation(elem_type);
                    auto stored_val = &index_write_ins->getValueWritten();
                    switch (mins->getKind()) {
                        case llvm::memoir::INDEX_WRITE_STRUCT_REF:
                        case llvm::memoir::INDEX_WRITE_COLLECTION_REF: {
                            auto replPtr = replacementMapping.at(stored_val);
                            auto bc_inst = builder.CreateBitCast(replPtr, i8StarTy);
                            auto storeInst = builder.CreateStore(bc_inst, gep);
                            replacementMapping[&ins] = storeInst;
                            break;
                        }
                        default: {
                            auto storeInst = builder.CreateStore(stored_val, gep);
                            replacementMapping[&ins] = storeInst;
                            break;
                        }
                    }
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
                    assert(false && "Assoc. array should have been lowered down already");

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
                    assert(false && "Assoc. array should have been lowered down already");
                case llvm::memoir::DELETE_STRUCT:
                    Utility::debug() << "DELETE_STRUCT not yet implemented. TODO \n";
                    break;
                case llvm::memoir::JOIN:
                case llvm::memoir::SLICE:
                    assert(false && "sequences should have been lowered down already");
                case llvm::memoir::DELETE_COLLECTION:
                    Utility::debug() << "DELETE_STRUCT not yet implemented. TODO \n";
                    break;
                case llvm::memoir::ASSERT_STRUCT_TYPE:
                case llvm::memoir::ASSERT_COLLECTION_TYPE:
                case llvm::memoir::SET_RETURN_TYPE:
                    Utility::debug() << "TODO Interprocedural \n";
                    break;
                case llvm::memoir::NONE:
                    break;
            }

        }

        auto node = DT.getNode(bb);
        for (auto child: node->getChildren()) {
            auto dominated = child->getBlock();
            BasicBlockTransformer(DT,
                                  dominated,
                                  phiNodesReplacementStruct,
                                  phiNodesReplacementCollection);
        }
    }

}