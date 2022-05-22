#include "Parser.hpp"
#include "types.hpp"
#include <functional>

using namespace object_lowering;

Parser::Parser(Module &M, Noelle *noelle, ModulePass *mp, PointerType *objectStar)
  : M(M),
    noelle(noelle),
    mp(mp) ,
    objectStar(objectStar){
    // Do initialization.
}

object_lowering::AnalysisType* Parser::parseTypeCallInst(CallInst *ins, std::set<PHINode*> &visited) {
    // return a cached AnalysisType*
    if (analysisTypeMap.find(ins) != analysisTypeMap.end()) {
        return analysisTypeMap[ins];
    }    
    
    auto callee = ins->getCalledFunction();
    if (!callee) {
        errs() << "Unrecognized indirect call" << *ins << "\n";
        assert(false);
    }
    auto n = callee->getName().str();
    if (!isObjectIRCall(n)) {
        errs() << "Unrecognized function call " << *ins << "\n";
        assert(false);
    }

    AnalysisType* a_type; // the Type* of this CallInst will be reconstructed into an AnalysisType*

    switch (FunctionNamesToObjectIR[n])
    {
        case OBJECT_TYPE: {
            std::vector<object_lowering::AnalysisType*> typeVec;
            for(auto arg = ins->arg_begin() + 1; arg != ins->arg_end(); ++arg)
            {
                auto ins = arg->get();
                object_lowering::AnalysisType* type;
                std::function<void(CallInst*)> call_back = [&](CallInst* ci) {
                    type = parseTypeCallInst(ci,visited);
                };
                parseType(dyn_cast_or_null<Instruction>(ins), call_back,visited);
                typeVec.push_back(type);
            }
            auto tmp = new ObjectType();
            tmp->fields = typeVec;
            a_type = tmp;
            break;
        }
        case NAME_OBJECT_TYPE: {
            std::vector<object_lowering::AnalysisType*> typeVec;
            auto name = fetchString(ins->arg_begin()->get());
            // the same as BUILD_OBJECT, except start from the 3rd argument (skip the name & # of args)
            for(auto arg = ins->arg_begin() + 2; arg != ins->arg_end(); ++arg)
            {
                auto ins = arg->get();
                object_lowering::AnalysisType* type;
//                std::set<PHINode*> visited;
                std::function<void(CallInst*)> call_back = [&](CallInst* ci) {
                    type = parseTypeCallInst(ci,visited);
                };
                parseType(dyn_cast_or_null<Instruction>(ins), call_back,visited);
                typeVec.push_back(type);
            }
            auto tmp = new ObjectType();
            tmp->name = name;
            tmp->fields = typeVec;
            a_type = tmp;
            break;
        }
        case POINTER_TYPE: {
            auto pointsToVal = ins->getArgOperand(0);
            object_lowering::AnalysisType* type;
            std::set<PHINode*> visited;
            std::function<void(CallInst*)> call_back = [&](CallInst* ci) {
                type = parseTypeCallInst(ci,visited);
            };
            parseType(pointsToVal, call_back,visited);
            auto tmp = new APointerType();
            tmp->pointsTo = type;
            a_type = tmp;
            break;
        }
        case GET_NAME_TYPE: {
            auto name = fetchString(ins->getArgOperand(0));
            auto tmp = new StubType(name);
            a_type = tmp;
            break;
        }
        case UINT64_TYPE:
            a_type = new object_lowering::IntegerType(64, false); break;
        case UINT32_TYPE:
            a_type = new object_lowering::IntegerType(32, true); break;
        case UINT16_TYPE:
            a_type = new object_lowering::IntegerType(16, true); break;
        case UINT8_TYPE:
            a_type = new object_lowering::IntegerType(8, true); break;
        case INT64_TYPE:
            a_type = new object_lowering::IntegerType(64, false); break;
        case INT32_TYPE:
            a_type = new object_lowering::IntegerType(32, false); break;
        case INT16_TYPE:
            a_type = new object_lowering::IntegerType(16, false); break;
        case INT8_TYPE:
            a_type = new object_lowering::IntegerType(8, false); break;
        case FLOAT_TYPE:
            a_type = new object_lowering::FloatType(); break;
        case DOUBLE_TYPE:
            a_type = new object_lowering::DoubleType(); break;
        default:
            errs() <<"the switch should cover everything this is wrong\n";
            errs() << *ins;
            assert(false);
            break;
    }
    analysisTypeMap[ins] = a_type;
    return a_type;
} // endof parseTypeCallInst

std::string Parser::fetchString(Value* ins) {
    if(auto firstargGep = dyn_cast<GetElementPtrInst>(ins)) {
        if(firstargGep->getNumIndices() == 2 && firstargGep->hasAllZeroIndices()) {
            if (auto glob_var = dyn_cast<GlobalVariable>(firstargGep->getPointerOperand())) {
                if (auto cda = dyn_cast<ConstantDataArray>(glob_var->getInitializer())) {
                    auto str = cda->getAsCString().str();
                    return str;
                }
            }
        }
    }
    errs() << "Not able to fetch str from " << *ins << "\n";
    assert(false);
}

ObjectWrapper *Parser::parseObjectWrapperChain(Value* i, std::set<PHINode*> &visited)
{
    errs() << "Trying to obtain object wrapper for " << *i <<"\n";

    // return the cached objectWrapper, if it exists
    if (buildObjMap.find(i)!=buildObjMap.end())
    {
        return buildObjMap[i];
    }
//    if(dyn_cast<Argument>(i))
//    {
//        errs() <<"The object is passed in \n ";
//        for(auto u: i->users()) {
//            if (auto ins = dyn_cast_or_null<CallInst>(u)) {
//                auto callee = ins->getCalledFunction();
//                if (!callee) continue;
//                auto n = callee->getName().str();
//                if (n == ObjectIRToFunctionNames[ASSERT_TYPE]) {
//                    // use parseType to retreive the type info from the first operand
//                    auto newTypeInst = ins->getArgOperand(0);
//                    object_lowering::AnalysisType* a_type;
//                    std::function<void(CallInst*)> call_back = [&](CallInst* ci) {
//                        a_type = parseTypeCallInst(ci,visited);
//                    };
//                    parseType(newTypeInst, call_back, visited);
//                    // make sure it is an ObjectType
//                    assert(a_type);
//                    if(a_type->getCode() != ObjectTy) assert(false);
//                    auto* objt = (ObjectType*) a_type;
//                    buildObjMap[i] = new ObjectWrapper(objt);
//                }
//            }
//        }
//    }
//    else {
        ObjectWrapper *objw;
        std::function<void(CallInst *)> call_back = [&](CallInst *ci) {
            //errs() << "Field Wrapper found function " << *ci << "\n";
            objw = parseObjectWrapperInstruction(ci, visited);
        };
        parseType(i, call_back, visited);
        buildObjMap[i] = objw;
//    }

    errs() << "obtained object wrapper with address " << objw ;
    errs() << "and the addresss of the inner object type is " << objw->innerType << "\n";
    return buildObjMap[i];
}

ObjectWrapper *Parser::parseObjectWrapperInstruction(CallInst *i, std::set<PHINode*> &visited) {
    if (buildObjMap.find(i)!=buildObjMap.end())
    {
        return buildObjMap[i];
    }
    auto funcName = i->getCalledFunction()->getName().str();
    if(funcName == ObjectIRToFunctionNames[BUILD_OBJECT]){
        auto typeArg = i->getArgOperand(0); // this should be a loadInst from a global Type**
        AnalysisType* type;
        std::function<void(CallInst*)> callback = [&](CallInst* ci) {
            type = parseTypeCallInst(ci,visited);
        };
        parseType(typeArg,callback,visited);
        errs() << "Obtained AnalysisType for " << *i <<"\n";

        if(type->getCode() != ObjectTy) {
            //errs() << "It's not an object";
            assert(false);
        }
        auto* objt = (ObjectType*) type;
        buildObjMap[i]= new ObjectWrapper(objt);
    }
    else if(funcName == ObjectIRToFunctionNames[READ_POINTER])
    {
        errs() << "Processing the type of the inner pointer for " << *i << "\n\n";
        FieldWrapper* fw;
        std::function<void(CallInst*)> call_back = [&](CallInst* ci) {
            fw = parseFieldWrapperIns(ci,visited);
        };
        parseType(i->getArgOperand(0), call_back,visited);
        buildObjMap[i]= new ObjectWrapper(fw->objectType);
    }
    else if (funcName == ObjectIRToFunctionNames[ASSERT_TYPE])
    {
        auto newTypeInst = i->getArgOperand(0);
        errs() << "Processing the assert type for " << *newTypeInst << "\n\n";
        object_lowering::AnalysisType* a_type;
        std::function<void(CallInst*)> call_back = [&](CallInst* ci) {
            a_type = parseTypeCallInst(ci,visited);
        };
        parseType(newTypeInst, call_back, visited);
        // make sure it is an ObjectType
        assert(a_type);
        if(a_type->getCode() != ObjectTy) assert(false);
        auto* objt = (ObjectType*) a_type;
        objt->getLLVMRepresentation(M);
        buildObjMap[i]= new ObjectWrapper(objt);
    }
    else if(clonedFunctionReturnTypes.find(i->getCalledFunction())!= clonedFunctionReturnTypes.end())
    {
        auto retType = clonedFunctionReturnTypes[i->getCalledFunction()];
        buildObjMap[i]= new ObjectWrapper(retType);
    }
    else
    {
        assert(false);
    }
    return buildObjMap[i];
}


void Parser::parseType(Value *ins, const std::function<void(CallInst*)>& callback, std::set<PHINode*>& visited) {
   //errs()<<*ins << "is being called by parseType\n";

    if (auto callins = dyn_cast_or_null<CallInst>(ins)) {
        callback(callins);
    }
    else if (auto storeIns = dyn_cast_or_null<StoreInst>(ins)) {
        parseTypeStoreInst(storeIns,callback,visited);
    }
    else if(auto loadIns = dyn_cast_or_null<LoadInst>(ins)) {
        parseTypeLoadInst(loadIns,callback,visited);
    }
    else if (auto allocaIns = dyn_cast_or_null<AllocaInst>(ins)) {
        parseTypeAllocaInst(allocaIns,callback,visited);
    } else if (auto gv = dyn_cast_or_null<GlobalValue>(ins)) {
        parseTypeGlobalValue(gv,callback,visited);
    } else if (auto phiInst = dyn_cast_or_null<PHINode>(ins)) {
        // if the value has been visited, do nothing
        if(visited.find(phiInst) != visited.end()) return;
        // otherwise, vist the children
        visited.insert(phiInst);
        for (auto& val: phiInst->incoming_values()) {
            if(dyn_cast<ConstantPointerNull>(val.get())) continue;
            parseType(val.get(), callback, visited);
        }
    } else if (auto arg = dyn_cast_or_null<Argument>(ins))
    {
        for(auto u: arg->users()) {
            if (auto call_ins = dyn_cast_or_null<CallInst>(u)) {
                auto callee = call_ins->getCalledFunction();
                if (!callee) continue;
                auto n = callee->getName().str();
                if (n == ObjectIRToFunctionNames[ASSERT_TYPE]) {
                    callback(call_ins);
                }
            }
        }

    } else if (!ins) {
        //errs() << "i think this is a nullptr\n";
        assert(false); 
    } else {
        errs() << "parseType: Unrecognized instruction" << *ins <<"\n";
        assert(false);
    }
}



void Parser::parseTypeStoreInst(StoreInst *ins, const std::function<void(CallInst*)>& callback, std::set<PHINode*> &visited) {
    auto valOp = ins->getValueOperand();
    parseType(dyn_cast_or_null<Instruction>(valOp), callback,visited);
}

void Parser::parseTypeLoadInst(LoadInst *ins, const std::function<void(CallInst*)>& callback, std::set<PHINode*> &visited) {
    auto ptrOp = ins->getPointerOperand();
    parseType(ptrOp,callback,visited);
}

void Parser::parseTypeAllocaInst(AllocaInst *ins, const std::function<void(CallInst*)>& callback, std::set<PHINode*> &visited) {
    for(auto u: ins->users()) {
        if(auto i = dyn_cast_or_null<StoreInst>(u)) {
            return parseType(i,callback,visited);
        }
    }
    //errs() << "Didn't find any store instruction uses for the instruction" <<*ins;
    assert(false);
}

void Parser::parseTypeGlobalValue(GlobalValue *gv, const std::function<void(CallInst*)>& callback, std::set<PHINode*> &visited) {
    for(auto u: gv->users()) {
        if(auto i = dyn_cast_or_null<StoreInst>(u)) {
            return parseType(i,callback,visited);
        }
    }
    //errs() << "Didn't find any store instruction uses for the gv" << *gv;
    assert(false);
}



FieldWrapper* Parser::parseFieldWrapperIns(CallInst* i, std::set<PHINode*> &visited)
{
    errs() << "Parsing field wrapper for " << *i << "\n";

    auto callee = i->getCalledFunction();
    if (!callee) {
        errs() << "Unrecognized indirect call" << *i << "\n";
        assert(false);
    }
    auto n = callee->getName().str();
    if(n != ObjectIRToFunctionNames[GETOBJECTFIELD]) {
        errs() << "Def use chain gave us the wrong function?" << *i;
        assert(false);
    }
    // get the arguments out of @getObjectField
    auto objPtr = i->getArgOperand(0);
    auto llvmFieldIndex =  i->getArgOperand(1);
    auto CI = dyn_cast_or_null<ConstantInt>(llvmFieldIndex);
    assert(CI);
    int64_t fieldIndex = CI->getSExtValue();
    // grab the ObjectWrapper*
    auto objw = parseObjectWrapperChain(objPtr, visited);
    errs() << "Obtained Field Wrapper AnalysisType for " << *i <<"\n\n";
    auto fieldwrapper = new FieldWrapper();
    fieldwrapper->baseObjPtr = objPtr;
    fieldwrapper->fieldIndex = fieldIndex; // NOLINT(cppcoreguidelines-narrowing-conversions)
    fieldwrapper->objectType = objw->innerType; 
    return fieldwrapper;
}

FieldWrapper* Parser::parseFieldWrapperChain(Value* i, std::set<PHINode*> &visited)
{
    //TODO: Cache???

    FieldWrapper* fw;
    std::function<void(CallInst*)> call_back = [&](CallInst* ci) {
        fw = parseFieldWrapperIns(ci,visited);
    };
    parseType(i, call_back,visited);
    return fw;
}

void Parser::setClonedFunctionReturnTypes(std::map<Function *, ObjectType *> &clonedFunctionReturnTypes) {
 this->clonedFunctionReturnTypes = clonedFunctionReturnTypes;
}
