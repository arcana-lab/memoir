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
clang++ -I/home/pze9918/object-ir/runtime/include -std=c++17 -O1 -Xclang -disable-llvm-passes -emit-llvm -c main.cpp -o build/main.bc


llvm-link build/main.bc /home/pze9918/object-ir/runtime/build/objectir.bc -o build/all_in_one.bc

noelle-norm build/all_in_one.bc -o build/all_in_one.bc

noelle-load -load ../../../compiler/passes/build/lib/ObjectLowering.so -ObjectLowering build/all_in_one.bc -o build/all_in_one.bc

llc -filetype=obj build/all_in_one.bc -o build/all_in_one.o
clang++ build/all_in_one.o -o build/all_in_one
```

to debug - invoke a different bash script:
```
noelle-load-gdb -load ../../../compiler/passes/build/lib/ObjectLowering.so -ObjectLowering build/all_in_one.bc -o build/all_in_one.bc
```

## overall
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
cd unit ; make links
TESTS="test_0"
make test

get_or_insert_function(funcTy, "malloc") // will resolve null ptr error

IRbuilder createCall: first 2 args are required, rest are optional (twine is used to name the new variable)
ins->getNextNode can be used when instantiating the builder s.t. the new call is inserted after the given `ins`
the API for getting type (eg int32) from the builder is cleaner than getting it from the context
struct type will work for the size but not off the stack


## 04-20
- automatic DCE after our pass (yes by invoking noelle fixed point -dce)
- best practice for deleting and reinserting tons of instructions: try noelle first ; then remove uses
- object that doesnt escape the function can be on the stack. we will know when the obj is dead and insert a free

- malloc and gep need to respect alignment
- gep has a default alignment to 4 bytes (?) depending on architecture ; the param to gep is a factor to the alignment

- we will need to analyze whether an Obj escapes a function. if it does not, then it can be alloca'd on stack. o.w., malloc'd on the heap

# other
You can build a single test by running `tests/scripts/compile.sh <path to test>` (a directory like test_0) and then run it by executing the all_in_one binary in the build/ dir of a given test. They are meant to test different functions of the ObjectIR API and/or the passes. Right now it just has a simple object test and an array of structs test (for the array of structs -> struct of arrays pass)
