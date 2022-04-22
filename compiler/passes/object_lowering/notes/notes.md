# compilation cmds
to compile the pass:
```
cd objectlowering
./run_me.sh
```

to compile the test:
```
rm -rf build
mkdir -p build
clang++ -I/home/pjp2292/object-ir/runtime/include -std=c++17 -emit-llvm -c main.cpp -o build/main.bc

llvm-link build/main.bc /home/pjp2292/object-ir/runtime/build/objectir.bc -o build/all_in_one.bc

noelle-norm build/all_in_one.bc -o build/all_in_one.bc

noelle-load -load ../../../compiler/passes/build/lib/ObjectLowering.so -ObjectLowering build/all_in_one.bc -o build/all_in_one.bc
```

to debug - invoke a different bash script:
```
noelle-load-gdb -load ../../../compiler/passes/build/lib/ObjectLowering.so -ObjectLowering build/all_in_one.bc -o build/all_in_one.bc
```

# Algos
0. collect all of the buildObject insts
1. call buildObject inst => parameter is load inst
2. load inst => memory location is type\*
look @ uses of Type\*
3. there exists a store somewhere into the type\*
4. the value of the store will be the getType callInst

goal: %0 => Object{int64, int64, int64}
 
implementation
- process getObjectfield
- collect writes and reads

- replace call BuildObject with malloc ; rm subsequent store
- rm everything with fields
- turn writes and reads into geps


# tests to write
- instantiate multiple objects of the same type (might have mutliple stores and loads to the Type\*)
- conditional branches w/ object\* assignment
- printing objects (and types)
- global type defs -  from tommy

# questions
- best practice for casts of non-llvm classes (use dynamic_cast for non-llvm classes). always use object_lowering::Type
- automatic DCE after our pass (yes by invoking noelle fixed point -dce)
- best practice for deleting and reinserting tons of instructions: try noelle first ; then remove uses
- object that doesnt escape the function can be on the stack. we will know when the obj is dead and insert a free

- malloc and gep need to respect alignment
- gep has a default alignment to 4 bytes (?) depending on architecture ; the param to gep is a factor to the alignment

### creating a nested obj
```
Type* ty = createType(getU64Ty(), innerTy); // nested object w/ Object ptr inside it

obj = .... buildObj ...

// reading an int from the inner obj
Object* Inobj = getObject(getField(obj, 1));
uint64 val = readUnit64(getObjField(Inobj, 0));
```

- we will need to analyze whether an Obj escapes a function. if it does not, then it can be alloca'd on stack. o.w., malloc'd on the heap

# other
You can build a single test by running `tests/scripts/compile.sh <path to test>` (a directory like test_0) and then run it by executing the all_in_one binary in the build/ dir of a given test. They are meant to test different functions of the ObjectIR API and/or the passes. Right now it just has a simple object test and an array of structs test (for the array of structs -> struct of arrays pass)

# running test_0
```
[pjp2292@moore unit]$ make clean ; make
rm -rf */build
../scripts/compile.sh test_0/ test_aos/ ;
~/object-ir/tests/unit/test_0 ~/object-ir/tests/unit
Building test: test_0/
make[1]: Entering directory '/home/pjp2292/object-ir/tests/unit/test_0'
mkdir -p build
clang++ -I/home/pjp2292/object-ir/runtime/include -std=c++17 -emit-llvm -c main.cpp -o build/main.bc
llvm-link build/main.bc /home/pjp2292/object-ir/runtime/build/objectir.bc -o build/all_in_one.bc
/home/pjp2292/object-ir/compiler/scripts/optimize.sh build/all_in_one.bc
opt -basicaa -load /home/pjp2292/object-ir/compiler/noelle/install/lib/libSvf.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/libCudd.so -stat=false -mem2reg -simplifycfg-sink-common=false -break-constgeps -lowerswitch -mergereturn --break-crit-edges -loop-simplify -lcssa -indvars --functionattrs --rpo-functionattrs build/all_in_one.bc -o build/all_in_one.bc
Lower Objects (I: build/all_in_one.bc, O: build/all_in_one.bc)
opt -load /home/pjp2292/object-ir/compiler/noelle/install/lib/CallGraph.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/libSvf.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/libCudd.so -stat=false -load /home/pjp2292/object-ir/compiler/noelle/install/lib/libSCAFUtilities.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/libMemoryAnalysisModules.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/AllocAA.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/TalkDown.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/CallGraph.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/PDGAnalysis.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/Architecture.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/BasicUtilities.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/Task.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/DataFlow.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/HotProfiler.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/LoopStructure.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/Invariants.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/InductionVariables.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/Loops.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/Scheduler.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/OutlinerPass.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/MetadataManager.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/LoopTransformer.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/Noelle.so -disable-basicaa -globals-aa -cfl-steens-aa -tbaa -scev-aa -cfl-anders-aa --objc-arc-aa -basic-loop-aa -scev-loop-aa -auto-restrict-aa -intrinsic-aa -global-malloc-aa -pure-fun-aa -semi-local-fun-aa -phi-maze-aa -no-capture-global-aa -no-capture-src-aa -type-aa -no-escape-fields-aa -acyclic-aa -disjoint-fields-aa -field-malloc-aa -loop-variant-allocation-aa -std-in-out-err-aa -array-of-structures-aa -kill-flow-aa -callsite-depth-combinator-aa -unique-access-paths-aa -llvm-aa-results -scalar-evolution -loops -domtree -postdomtree -noellescaf -noellesvf -load /home/pjp2292/object-ir/compiler/passes/build/lib/ObjectLowering.so -ObjectLowering build/all_in_one.bc -o build/all_in_one.bc
llc -filetype=obj build/all_in_one.bc -o build/all_in_one.o
clang++ build/all_in_one.o -o build/all_in_one
make[1]: Leaving directory '/home/pjp2292/object-ir/tests/unit/test_0'
~/object-ir/tests/unit
~/object-ir/tests/unit/test_aos ~/object-ir/tests/unit
Building test: test_aos/
make[1]: Entering directory '/home/pjp2292/object-ir/tests/unit/test_aos'
mkdir -p build
clang++ -I/home/pjp2292/object-ir/runtime/include -std=c++17 -emit-llvm -c main.cpp -o build/main.bc
llvm-link build/main.bc /home/pjp2292/object-ir/runtime/build/objectir.bc -o build/all_in_one.bc
/home/pjp2292/object-ir/compiler/scripts/optimize.sh build/all_in_one.bc
opt -basicaa -load /home/pjp2292/object-ir/compiler/noelle/install/lib/libSvf.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/libCudd.so -stat=false -mem2reg -simplifycfg-sink-common=false -break-constgeps -lowerswitch -mergereturn --break-crit-edges -loop-simplify -lcssa -indvars --functionattrs --rpo-functionattrs build/all_in_one.bc -o build/all_in_one.bc
Lower Objects (I: build/all_in_one.bc, O: build/all_in_one.bc)
opt -load /home/pjp2292/object-ir/compiler/noelle/install/lib/CallGraph.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/libSvf.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/libCudd.so -stat=false -load /home/pjp2292/object-ir/compiler/noelle/install/lib/libSCAFUtilities.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/libMemoryAnalysisModules.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/AllocAA.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/TalkDown.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/CallGraph.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/PDGAnalysis.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/Architecture.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/BasicUtilities.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/Task.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/DataFlow.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/HotProfiler.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/LoopStructure.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/Invariants.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/InductionVariables.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/Loops.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/Scheduler.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/OutlinerPass.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/MetadataManager.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/LoopTransformer.so -load /home/pjp2292/object-ir/compiler/noelle/install/lib/Noelle.so -disable-basicaa -globals-aa -cfl-steens-aa -tbaa -scev-aa -cfl-anders-aa --objc-arc-aa -basic-loop-aa -scev-loop-aa -auto-restrict-aa -intrinsic-aa -global-malloc-aa -pure-fun-aa -semi-local-fun-aa -phi-maze-aa -no-capture-global-aa -no-capture-src-aa -type-aa -no-escape-fields-aa -acyclic-aa -disjoint-fields-aa -field-malloc-aa -loop-variant-allocation-aa -std-in-out-err-aa -array-of-structures-aa -kill-flow-aa -callsite-depth-combinator-aa -unique-access-paths-aa -llvm-aa-results -scalar-evolution -loops -domtree -postdomtree -noellescaf -noellesvf -load /home/pjp2292/object-ir/compiler/passes/build/lib/ObjectLowering.so -ObjectLowering build/all_in_one.bc -o build/all_in_one.bc
llc -filetype=obj build/all_in_one.bc -o build/all_in_one.o
clang++ build/all_in_one.o -o build/all_in_one
make[1]: Leaving directory '/home/pjp2292/object-ir/tests/unit/test_aos'
~/object-ir/tests/unit
[pjp2292@moore unit]$ 
```



