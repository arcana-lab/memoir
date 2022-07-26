#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace object_lowering {

/*
// * Utility functions
// */
//bool isObjectIRCall(std::string functionName);

///*
// * Enum of object ir functions
// */
//enum ObjectIRFunc {
//  // types
//  NAME_OBJECT_TYPE,
//  POINTER_TYPE,
//  GET_NAME_TYPE,
//  OBJECT_TYPE,
//  ARRAY_TYPE,
//  UNION_TYPE,
//  INTEGER_TYPE,
//  UINT64_TYPE,
//  UINT32_TYPE,
//  UINT16_TYPE,
//  UINT8_TYPE,
//  INT64_TYPE,
//  INT32_TYPE,
//  INT16_TYPE,
//  INT8_TYPE,
//  FLOAT_TYPE,
//  DOUBLE_TYPE,
//  // builds
//  BUILD_OBJECT,
//  BUILD_ARRAY,
//  BUILD_UNION,
//  // geps
//  GETOBJECTFIELD,
//  GETARRAYELEMENT,
//  GETUNIONMEMBER,
//  // asserts
//  ASSERT_TYPE,
//  SET_RETURN_TYPE,
//  // accessors
//  READ_OBJECT,
//  WRITE_OBJECT,
//  READ_ARRAY,
//  WRITE_ARRAY,
//  READ_UNION,
//  WRITE_UNION,
//  READ_INTEGER,
//  WRITE_INTEGER,
//  READ_UINT64,
//  WRITE_UINT64,
//  READ_UINT32,
//  WRITE_UINT32,
//  READ_UINT16,
//  WRITE_UINT16,
//  READ_UINT8,
//  WRITE_UINT8,
//  READ_INT64,
//  WRITE_INT64,
//  READ_INT32,
//  WRITE_INT32,
//  READ_INT16,
//  WRITE_INT16,
//  READ_INT8,
//  WRITE_INT8,
//  READ_FLOAT,
//  WRITE_FLOAT,
//  READ_DOUBLE,
//  WRITE_DOUBLE,
//  WRITE_POINTER,
//  READ_POINTER,
//  // other
//  DELETE_OBJECT
//};
//
///*
// * Mapping from object ir function enum to function name as
// * string
// */
//static std::unordered_map<ObjectIRFunc, std::string>
//    ObjectIRToFunctionNames = {
//      // types
//      { NAME_OBJECT_TYPE, "nameObjectType" },
//      { POINTER_TYPE, "getPointerType" },
//      { GET_NAME_TYPE, "getNamedType" },
//      { OBJECT_TYPE, "getObjectType" },
//      { ARRAY_TYPE, "getArrayType" },
//      { UNION_TYPE, "getUnionType" },
//      { INTEGER_TYPE, "getIntegerType" },
//      { UINT64_TYPE, "getUInt64Type" },
//      { UINT32_TYPE, "getUInt32Type" },
//      { UINT16_TYPE, "getUInt16Type" },
//      { UINT8_TYPE, "getUInt8Type" },
//      { INT64_TYPE, "getInt64Type" },
//      { INT32_TYPE, "getInt32Type" },
//      { INT16_TYPE, "getInt16Type" },
//      { INT8_TYPE, "getInt8Type" },
//      { FLOAT_TYPE, "getFloatType" },
//      { DOUBLE_TYPE, "getDoubleType" },
//      // builds
//      { BUILD_OBJECT, "buildObject" },
//      { BUILD_ARRAY, "buildArray" },
//      { BUILD_UNION, "buildUnion" },
//      // INCOMPLETE
//      // geps
//      { GETOBJECTFIELD, "getObjectField" },
//      // asserts
//      { ASSERT_TYPE, "assertType" },
//      { SET_RETURN_TYPE, "setReturnType" },
//      // accessors
//      { READ_UINT64, "readUInt64" },
//      { WRITE_UINT64, "writeUInt64" },
//      { READ_UINT32, "readUInt32" },
//      { WRITE_UINT32, "writeUInt32" },
//      {READ_POINTER, "readPointer"},
//      {WRITE_POINTER, "writePointer" },
//      // other
//      {DELETE_OBJECT, "deleteObject"}
//    };
//
//static std::unordered_map<std::string, ObjectIRFunc>
//    FunctionNamesToObjectIR = {
//      { "nameObjectType", NAME_OBJECT_TYPE },
//      { "getPointerType", POINTER_TYPE },
//      { "getNamedType", GET_NAME_TYPE },
//      { "getObjectType", OBJECT_TYPE },
//      { "getArrayType", ARRAY_TYPE },
//      { "getUnionType", UNION_TYPE },
//      { "getIntegerType", INTEGER_TYPE },
//      { "getUInt64Type", UINT64_TYPE },
//      { "getUInt32Type", UINT32_TYPE },
//      { "getUInt16Type", UINT16_TYPE },
//      { "getUInt8Type", UINT8_TYPE },
//      { "getInt64Type", INT64_TYPE },
//      { "getInt32Type", INT32_TYPE },
//      { "getInt16Type", INT16_TYPE },
//      { "getInt8Type", INT8_TYPE },
//      { "getFloatType", FLOAT_TYPE },
//      { "getDoubleType", DOUBLE_TYPE },
//      { "buildObject", BUILD_OBJECT },
//      { "buildArray", BUILD_ARRAY },
//      { "buildUnion", BUILD_UNION },
//      // INCOMPLETE
//      // geps
//      { "getObjectField", GETOBJECTFIELD },
//      // asserts
//      { "assertType", ASSERT_TYPE },
//      { "setReturnType", SET_RETURN_TYPE },
//      // accessors
//      { "readUInt64", READ_UINT64 },
//      { "writeUInt64", WRITE_UINT64 },
//      { "readUInt32", READ_UINT32 },
//      { "writeUInt32", WRITE_UINT32 },
//      {"readPointer", READ_POINTER},
//      {"writePointer", WRITE_POINTER},
//      { "writeObject", WRITE_OBJECT},
//      // other
//      {"deleteObject", DELETE_OBJECT}
//    };

} // namespace object_lowering
