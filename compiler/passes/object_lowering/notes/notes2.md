# code notes
```
callback : CallInst* -> Void

buildObject
1. ObjectWrapper* parseObjectWrapperInstruction(CallInst *i)
2. parseObjWrapper calls parseType w/ a callback 
   which calls parseTypeCallInst on the input inst
3. parseType "dispatches" until it reaches the next callInst (@getObjectType)
4. parseType invokes the callback
5. parseTypeCallInst invokes parseType, with a recursive callback,
   to construct the type tree

reads, writes
1. invoke parseType (on the parameter to R/W, which is a load from a field)
   with a callback to:
   FieldWrapper* ObjectLowering::parseFieldWrapperIns(CallInst* i)
2. parseType "dispatches" until it reaches @getObjectField
3. the callback to parseFieldWrapperIns will construct a FieldWrapper* from the information in the getObjectField call
```


# individual test run

```
rm -rf build
mkdir -p build
clang++ -I/home/pjp2292/object-ir/runtime/include -std=c++17 -O1 -Xclang -disable-llvm-passes -emit-llvm -c main.cpp -o build/main.bc


llvm-link build/main.bc /home/pjp2292/object-ir/runtime/build/objectir.bc -o build/all_in_one.bc

noelle-norm build/all_in_one.bc -o build/all_in_one.bc
noelle-load -load ../../../compiler/passes/build/lib/ObjectLowering.so -ObjectLowering build/all_in_one.bc -o build/all_in_one.bc

llc -filetype=obj build/all_in_one.bc -o build/all_in_one.o
clang++ build/all_in_one.o -o build/all_in_one
```

# output from running make test
```
mkdir -p build
clang++ -I/home/pjp2292/object-ir/runtime/include -std=c++17 -O1 -Xclang -disable-llvm-passes -emit-llvm -c main.cpp -o build/main.bc
llvm-link build/main.bc /home/pjp2292/object-ir/runtime/build/objectir.bc -o build/all_in_one.bc
/home/pjp2292/object-ir/compiler/scripts/optimize.sh build/all_in_one.bc
opt -basicaa -load /home/pjp2292/object-ir/compiler/noelle/install/lib/libSvf.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/libCudd.so -stat=false -mem2reg -simplifycfg-sink-common=false -break-constgeps -lowerswitch -mergereturn --break-crit-edges -loop-simplify -lcssa -indvars --functionattrs --rpo-functionattrs build/all_in_one.bc -o build/all_in_one.bc

opt -load /home/pjp2292/object-ir/compiler/noelle/install/lib/CallGraph.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/libSvf.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/libCudd.so -stat=false -load /home/pjp2292/object-ir/compiler/noelle/install/lib/libSCAFUtilities.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/libMemoryAnalysisModules.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/AllocAA.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/TalkDown.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/CallGraph.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/PDGAnalysis.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/Architecture.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/BasicUtilities.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/Task.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/DataFlow.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/HotProfiler.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/LoopStructure.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/Invariants.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/InductionVariables.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/Loops.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/Scheduler.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/OutlinerPass.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/MetadataManager.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/LoopTransformer.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/Noelle.so -disable-basicaa -globals-aa -cfl-steens-aa -tbaa -scev-aa -cfl-anders-aa --objc-arc-aa -basic-loop-aa -scev-loop-aa -auto-restrict-aa -intrinsic-aa -global-malloc-aa -pure-fun-aa -semi-local-fun-aa -phi-maze-aa -no-capture-global-aa -no-capture-src-aa -type-aa -no-escape-fields-aa -acyclic-aa -disjoint-fields-aa -field-malloc-aa -loop-variant-allocation-aa -std-in-out-err-aa -array-of-structures-aa -kill-flow-aa -callsite-depth-combinator-aa -unique-access-paths-aa -llvm-aa-results -scalar-evolution -loops -domtree -postdomtree -noellescaf -noellesvf -load /home/pjp2292/object-ir/compiler/passes/build/lib/ObjectLowering.so -ObjectLowering build/all_in_one.bc -o build/all_in_one.bc
```