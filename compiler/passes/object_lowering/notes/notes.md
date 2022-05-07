# compilation cmds
to compile the pass:
```
cd objectlowering
./run_me.sh
```

to compile the test:
```
make TESTS="test_recurse" clean
make TESTS="test_recurse" 
```
```
rm -rf build && mkdir -p build && clang++ -I/home/pze9918/object-ir/runtime/include -std=c++17 -O1 -Xclang -disable-llvm-passes -emit-llvm -c main.cpp -o build/main.bc && llvm-link build/main.bc /home/pze9918/object-ir/runtime/build/objectir.bc -o build/all_in_one.bc && noelle-norm build/all_in_one.bc -o build/all_in_one.bc && noelle-load -load ../../../compiler/passes/build/lib/ObjectLowering.so -ObjectLowering build/all_in_one.bc -o build/all_in_one.bc

llc -filetype=obj build/all_in_one.bc -o build/all_in_one.o
clang++ build/all_in_one.o -o build/all_in_one
```

to debug - invoke a different bash script:
```
noelle-load-gdb -load ../../../compiler/passes/build/lib/ObjectLowering.so -ObjectLowering build/all_in_one.bc -o build/all_in_one.bc
```

# notes and planning

## 05-05 coding
goal:
1. refactor type signature detection
2. create function clones w/ new type sigs
3. presentation: talk about design decisions and stack vs heap planning
4. define an abstraction for keeping track of cloned functions?
   1. we already have a map from old -> new Function*, which includes type signature
   2. we may want to map old -> new Argument*

## suggested methodology for interprocedural (from Wed meeting)
- clone function w/ new type signature // automatically creates value mapper
- value-map between old and new functions
- transform the new function ; checking dominator using the value-mapper w/ the old function

## interprocedural
OVERALL INTERPROCEDURAL ALGO
- [ ] collect all the type definitions from GVs
- [ ] scan over function signatures & detect any type signatures containing Object* or Field*
- [ ] do the cloning to setup these flagged function: look for assert types and assertReturnType to get the right typesignature
- [ ] analyze all (1) unflagged functions and (2) function clones. look for:
  - [ ] ObjectIR call instructions (eg build, read, write, like we do already)
  - [ ] any callinsts to flagged functions
  - [ ] return w/ objects
- [ ] transform analyzed functions with BBtransform
  - [ ] patch up callinsts to flagged/cloned functions
  - [ ] patch up returns
  - [ ] (these need to be done in Dominator order s.t. the object* used or defined by callInsts are replaced)
- [ ] remove dead code/functions

```
NOTES
we are currently replacing uses of `objectIRinstructions` with their shallow copies only for UINT64 reads
eg. uses of Object*, field*, etc were all used by the objectIR or phi nodes that we replaced, so we didnt need to replace their uses via llvm
however, base types like UINT64 do need this use-replacement

HOWEVER this will not be true for interprocedural:
1. arguments define Object* 
2. non-objectIR CallInsts can define Object*
3. non-objectIR CallInsts can use Object*
4. returns use Object*
```

## delete/free in OIR
- forward DFA: gen = buildObject; kill = deleteObject
- any objects still live in the return block must go on the heap

## merging in namedTypes (and pointer types)
```
first we collect all the global variables that matches type (i.,e done above)
Next we use the traditional trick of parsetype where nametypes are left as a stub
We also create a map from name to analysisType*
we loop through all the stubs, replacing it with actual typpes
     we have to be careful about infinite loops
we need to do something with bitcasting later
```


## various notes
- the GEP's second index will depend on the data layout of the obj accessed, right? 
- how does the GEP know the type of the data access, eg this GEP creates an int64 ptr?
  -> it that what llvmPtrType is for, as it is used in the PhiNode case?
  we may bitcast

```
analysis

parse buildObject => {`%4 = alloca %"struct.objectir::Object"*, align 8` ->  Type* C++ }

parse readField/writefields => <`%14 <- load object* from object *** `, object type, field number>

set of phi nodes that were traced + use it to check recursive dependence. 

Transform:

Go through dom tree and create copy of relavent instructions
maintain map between old instruction(ones using the runtime) and new instructions(ones that we create i.e. %4a, %14a ..... )
delete old instructions
we are done. 

map_ins = {old_ins -> new_ins}

loop through all build object:
	copy
loop through all phi ndoes:
	copy but leave the incoming values blank


loop through basic block b in dominator order:
  loop through ins in b;
    if b is alloca of object
       make copy of it 
       add to map_ins
    if b is a buildobject:
    	create a malloc
    	add malloc to map_ins
    if b is a phi node:
    	create new phi node
    	the incoming values must be in the map_ins
```

## Algo for parseType
0. collect all of the buildObject insts
1. call buildObject inst => parameter is load inst
2. load inst => memory location is type\*
look @ uses of Type\*
3. there exists a store somewhere into the type\*
4. the value of the store will be the getType callInst

goal: %0 => Object{int64, int64, int64}

# Meeting notes

## 04-27
- for running one test: `
cd unit ; make links
TESTS="test_0"
make test`

- IRbuilder createCall: first 2 args are required, rest are optional (twine is used to name the new variable)
- ins->getNextNode can be used when instantiating the builder s.t. the new call is inserted after the given `ins`
- the API for getting type (eg int32) from the builder is cleaner than getting it from the context
- struct type will work for the size but not off the stack(?)


## 04-20
- automatic DCE after our pass (yes by invoking noelle fixed point -dce)
- best practice for deleting and reinserting tons of instructions: try noelle first ; then remove uses
- object that doesnt escape the function can be on the stack. we will know when the obj is dead and insert a free

- malloc and gep need to respect alignment
- gep has a default alignment to 4 bytes (?) depending on architecture ; the param to gep is a factor to the alignment

- we will need to analyze whether an Obj escapes a function. if it does not, then it can be alloca'd on stack. o.w., malloc'd on the heap

# other
You can build a single test by running `tests/scripts/compile.sh <path to test>` (a directory like test_0) and then run it by executing the all_in_one binary in the build/ dir of a given test. They are meant to test different functions of the ObjectIR API and/or the passes. Right now it just has a simple object test and an array of structs test (for the array of structs -> struct of arrays pass)
