# MemOIR Description

This is the working description of the Memory Object IR (MOIR).

### Defining Struct Types
A struct type describes a memory object with a statically known, finite number of heterogeneously typed fields. Each struct type must be named, and can be referenced recursively with the `StructType(name)` method.

`DefineStructType(name, # of fields, *<field type, ...>*)`

`StructType(name)`

### Primitive Types
`IntegerType(bitwidth, is signed?)`

`UInt64Type()`

`UInt32Type()`

`UInt16Type()`

`UInt8Type()`

`Int64Type()`

`Int32Type()`

`Int16Type()`

`Int8Type()`

`BoolType()`

`FloatType()`

`DoubleType()`

`ReferenceType(referenced type)`,
a nullable references to memory objects, the main difference between this and C style pointers is that you are not allowed to index into reference types with pointer arithmetic. Doing so would result in an invalid program.

`TensorType(element type, # of dimensions)`,
a container that hold homogeneously typed elements contiguously in memory.

### Allocating Memory Objects
`allocateStruct(struct type)`

`allocateTensor(element type, # of dimensions, *<size of dimension, ...>*)`

### Accessing Fields
`getStructField(object, field index)`

`getTensorElement(tensor, *<index, ...>*)`

### Reading / Writing Fields
`readUInt64(field)`

`readUInt32(field)`

`readUInt16(field)`

`readUInt8(field)`

`readInt64(field)`

`readInt32(field)`

`readInt16(field)`

`readInt8(field)`

`readBool(field)`

`readInteger(field)`

`readFloat(field)`

`readDouble(field)`

`readReference(field)`

`writeUInt64(field, value)`

`writeUInt32(field, value)`

`writeUInt16(field, value)`

`writeUInt8(field, value)`

`writeInt64(field, value)`

`writeInt32(field, value)`

`writeInt16(field, value)`

`writeInt8(field, value)`

`writeBool(field, value)`

`writeInteger(field, value)`

`writeFloat(field, value)`

`writeDouble(field, value)`

`writeReference(field, object to reference)`

### Type Checking
`assertType(type, object)`

`assertFieldType(type, field)`

`setReturnType(type)`
