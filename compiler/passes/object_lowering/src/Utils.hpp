#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace object_lowering {

/*
 * Utility functions
 */
bool isObjectIRCall(std::string functionName);

/*
 * Enum of object ir functions
 */
enum ObjectIRFunc {
  // Type construction
  OBJECT_TYPE,
  NAME_OBJECT_TYPE,
  ARRAY_TYPE,
  NAME_ARRAY_TYPE,
  UNION_TYPE,
  NAME_UNION_TYPE,
  INTEGER_TYPE,
  UINT64_TYPE,
  UINT32_TYPE,
  UINT16_TYPE,
  UINT8_TYPE,
  INT64_TYPE,
  INT32_TYPE,
  INT16_TYPE,
  INT8_TYPE,
  FLOAT_TYPE,
  DOUBLE_TYPE,
  POINTER_TYPE,
  // Named types
  GET_NAME_TYPE,
  // Object construction
  BUILD_OBJECT,
  BUILD_ARRAY,
  BUILD_UNION,
  // Object destruction
  DELETE_OBJECT,
  // Object accesses
  GETOBJECTFIELD,
  // Array accesses
  GETARRAYELEMENT,
  // union accesses
  GETUNIONMEMBER,
  // Type checking
  ASSERT_TYPE,
  SET_RETURN_TYPE,
  // Field accesses
  // unsigned integer acces
  WRITE_UINT64,
  WRITE_UINT32,
  WRITE_UINT16,
  WRITE_UINT8,
  // signed integer access
  WRITE_INT64,
  WRITE_INT32,
  WRITE_INT16,
  WRITE_INT8,
  // floating point access
  WRITE_FLOAT,
  WRITE_DOUBLE,
  // pointer access
  WRITE_OBJECT,
  // unsigned integer access
  READ_UINT64,
  READ_UINT32,
  READ_UINT16,
  READ_UINT8,
  // signed integer access
  READ_INT64,
  READ_INT32,
  READ_INT16,
  READ_INT8,
  // floating point access
  READ_FLOAT,
  READ_DOUBLE,
  // pointer access
  READ_OBJECT,
  READ_POINTER,
  WRITE_POINTER
};

/*
 * Mapping from object ir function enum to function name as
 * string
 */
static std::unordered_map<ObjectIRFunc, std::string>
    ObjectIRToFunctionNames = {
      // types
      { OBJECT_TYPE, "getObjectType" },
      { NAME_OBJECT_TYPE, "nameObjectType" },
      { ARRAY_TYPE, "getArrayType" },
      { UNION_TYPE, "getUnionType" },
      // skip named versions
      { INTEGER_TYPE, "getIntegerType" },
      { UINT64_TYPE, "getUInt64Type" },
      { UINT32_TYPE, "getUInt32Type" },
      { UINT16_TYPE, "getUInt16Type" },
      { UINT8_TYPE, "getUInt8Type" },
      { INT64_TYPE, "getInt64Type" },
      { INT32_TYPE, "getInt32Type" },
      { INT16_TYPE, "getInt16Type" },
      { INT8_TYPE, "getInt8Type" },
      { FLOAT_TYPE, "getFloatType" },
      { DOUBLE_TYPE, "getDoubleType" },
      { POINTER_TYPE, "getPointerType" },
      // Named types
      { GET_NAME_TYPE, "getNamedType" },
      // Object construction
      { BUILD_OBJECT, "buildObject" },
      { BUILD_ARRAY, "buildArray" },
      { BUILD_UNION, "buildUnion" },
      // Object destruction
      { DELETE_OBJECT, "deleteObject"},
      // Object accesses
      { GETOBJECTFIELD, "getObjectField" },
      // skip Array accesses
      // skip Union accesses
      // Type checking
      { ASSERT_TYPE, "assertType" },
      { SET_RETURN_TYPE, "setReturnType" },
      // Field accesses - only those currently supported
      { WRITE_UINT64, "writeUInt64" },
      { WRITE_UINT32, "writeUInt32" },
      // pointer access
      { WRITE_OBJECT, "writeObject" },
      // ...
      { READ_UINT64, "readUInt64" },      
      { READ_UINT32, "readUInt32" },      
      // pointer access
      { READ_OBJECT, "readObject" },
      { READ_POINTER, "readPointer"},      
      { WRITE_POINTER, "writePointer" }      
    };

static std::unordered_map<std::string, ObjectIRFunc>
    FunctionNamesToObjectIR = {
      { "getObjectType", OBJECT_TYPE },
      { "nameObjectType", NAME_OBJECT_TYPE },
      { "getArrayType", ARRAY_TYPE },
      { "getUnionType", UNION_TYPE },
      // skip named versions
      { "getIntegerType", INTEGER_TYPE },
      { "getUInt64Type", UINT64_TYPE },
      { "getUInt32Type", UINT32_TYPE },
      { "getUInt16Type", UINT16_TYPE },
      { "getUInt8Type", UINT8_TYPE },
      { "getInt64Type", INT64_TYPE },
      { "getInt32Type", INT32_TYPE },
      { "getInt16Type", INT16_TYPE },
      { "getInt8Type", INT8_TYPE },
      { "getFloatType", FLOAT_TYPE },
      { "getDoubleType", DOUBLE_TYPE },
      { "getPointerType", POINTER_TYPE },
      // Named types
      { "getNamedType", GET_NAME_TYPE },
      // Object construction
      { "buildObject", BUILD_OBJECT },
      { "buildArray", BUILD_ARRAY },
      { "buildUnion", BUILD_UNION },
      // Object destruction
      {"deleteObject", DELETE_OBJECT},
      // Object acceses
      { "getObjectField", GETOBJECTFIELD },
       // skip Array accesses
      // skip Union accesses
      // Type checking
      { "assertType", ASSERT_TYPE },
      { "setReturnType", SET_RETURN_TYPE },
      // Field accesses - only those currently supported
      { "writeUInt64", WRITE_UINT64 },
      { "writeUInt32", WRITE_UINT32 },
      // pointer access
      { "writeObject", WRITE_OBJECT},
      // ...
      { "readUInt64", READ_UINT64 },
      { "readUInt32", READ_UINT32 },
      // pointer access
      { "readObject", READ_OBJECT },
      { "readPointer", READ_POINTER},
      { "writePointer", WRITE_POINTER}
    };

} // namespace object_lowering
