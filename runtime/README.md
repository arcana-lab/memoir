# ContainIR Description
Updated as of May 25, 2022

This is the working description of the ContainIR.

### Defining Container Types
nameContainerType(name, # of fields, *<field type, ...>*)

defineContainerType(# of fields, *<field type, ...>*)

getNamedType(name)

### Primitive Types
IntegerType(bitwidth, is signed?)

UInt64Type()

UInt32Type()

UInt16Type()

UInt8Type()

Int64Type()

Int32Type()

Int16Type()

Int8Type()

BoolType

FloatType()

DoubleType()

ReferenceType(referenced type)

TensorType(element type, # of dimensions)

### Allocating Containers
allocateContainer(container type)

allocateTensor(element type, # of dimensions, *<size of dimension, ...>*)

### Accessing Fields
getContainerField(object, field index)

getTensorElement(array, *<index, ...>*)

### Reading / Writing Fields
readUInt64(field)

readUInt32(field)

readUInt16(field)

readUInt8(field)

readInt64(field)

readInt32(field)

readInt16(field)

readInt8(field)

readBool(field)

readInteger(field)

readFloat(field)

readDouble(field)

readReference(field)

writeUInt64(field, value)

writeUInt32(field, value)

writeUInt16(field, value)

writeUInt8(field, value)

writeInt64(field, value)

writeInt32(field, value)

writeInt16(field, value)

writeInt8(field, value)

writeBool(field, value)

writeInteger(field, value)

writeFloat(field, value)

writeDouble(field, value)

writeReference(field, object to reference)

### Type Checking
assertType(type, object)

assertFieldType(type, field)

setReturnType(type)
