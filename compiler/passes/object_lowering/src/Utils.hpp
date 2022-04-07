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
  OBJECT_TYPE,
  ARRAY_TYPE,
  UNION_TYPE,
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
  BUILD_OBJECT,
  BUILD_ARRAY,
  BUILD_UNION
};

/*
 * Mapping from object ir function enum to function name as
 * string
 */
static std::unordered_map<ObjectIRFunc, std::string>
    ObjectIRToFunctionNames = {
      { OBJECT_TYPE, "getObjectType" },
      { ARRAY_TYPE, "getArrayType" },
      { UNION_TYPE, "getUnionType" },
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
      { BUILD_OBJECT, "buildObject" },
      { BUILD_ARRAY, "buildArray" },
      { BUILD_UNION, "buildUnion" }
    };

static std::unordered_map<std::string, ObjectIRFunc>
    FunctionNamesToObjectIR = {
      { "getObjectType", OBJECT_TYPE },
      { "getArrayType", ARRAY_TYPE },
      { "getUnionType", UNION_TYPE },
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
      { "buildObject", BUILD_OBJECT },
      { "buildArray", BUILD_ARRAY },
      { "buildUnion", BUILD_UNION }
    };

} // namespace object_lowering
