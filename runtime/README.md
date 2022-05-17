# Object IR Description

## Defining Types
defineObjectType(# of fields, ...)

defineTensorType(inner type, # of dimensions)

## Primitive Types
getUInt64Type()

getUInt32Type()

getUInt16Type()

getUInt8Type()

getInt64Type()

getInt32Type()

getInt16Type()

getInt8Type()

getFloatType()

getDoubleType()

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
