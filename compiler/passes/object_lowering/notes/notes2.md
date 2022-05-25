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

