# compilation cmds
to compile the pass:
```
cd objectlowering
./run_me.sh
```

to compile the test:
```
make TESTS="test_recurse" 
```

# notes and planning
- stack vs heap
  - DFA
  - using results of DFA
- intrinsic types
- field passing
- remove type GVs and loads
- copy constructors 

re: fieldpassing and copy constructors, we currently only look for function passing around Object* and assume they are pointers

## delete/free in OIR
- forward DFA: gen = buildObject; kill = deleteObject
- any objects still live in the return block must go on the heap

05-11 meeting notes
1. loop structures (guaranteed single entry and single exit)
   1. see loop abstractions in noelle (simone's slides). there might be mutliple latches but 1 guaranteed exit and entry (header)
2. DFA on the header and the basic blocks inside the loop
3. can have multiple latches: compute intersection of latches to see everything that might be alivei
4. phis are conservarive and can't kill anything 
5. we can assume ptrs for now but will need copy constructor later
6. assume that aobjects must be writte to/initialized before reading, so we don't need to clear a reused alloca

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
