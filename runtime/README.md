# ContainIR Description
Updated as of May 17, 2022

This is the working description of the ContainIR.

## Defining Container Types
nameContainerType(name, # of fields, ...)

defineContainerType(# of fields, ...)

getNamedType(name)

## Primitive Types
UInt64Type()

UInt32Type()

UInt16Type()

UInt8Type()

Int64Type()

Int32Type()

Int16Type()

Int8Type()

FloatType()

DoubleType()

ReferenceType(referenced type)

TensorType(element type, # of dimensions)

## Accessing Fields
getObjectField(object, field index)

getArrayElement(array, element index)

## Reading / Writing Fields
readUInt64(field)

readUInt32(field)

readUInt16(field)

readUInt8(field)

readInt64(field)

readInt32(field)

readInt16(field)

readInt8(field)

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

writeFloat(field, value)

writeDouble(field, value)

writeReference(field, object to reference)

## Type Checking
assertType(type, object)

assertFieldType(type, field)

setReturnType(type)
