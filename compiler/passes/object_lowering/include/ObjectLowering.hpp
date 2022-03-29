#pragma once

/*
 * Pass to perform lowering from object-ir to LLVM IR
 *
 * Author: Tommy McMichen
 * Created: March 29, 2022
 */

/*
 * Function names for creation of Types
 */
const std::string OBJECT_TYPE = "getObjectType",
                  ARRAY_TYPE = "getArrayType",
                  UNION_TYPE = "getUnionType",
                  INTEGER_TYPE = "getIntegerType",
                  UINT64_TYPE = "getUInt64Type",
                  UINT32_TYPE = "getUInt32Type",
                  UINT16_TYPE = "getUInt16Type",
                  UINT8_TYPE = "getUInt8Type",
                  INT64_TYPE = "getInt64Type",
                  INT32_TYPE = "getInt32Type",
                  INT16_TYPE = "getInt16Type",
                  INT8_TYPE = "getInt8Type",
                  FLOAT_TYPE = "getFloatType",
                  DOUBLE_TYPE = "getDoubleType";

/*
 * Function names for creation of Types
 */
const std::string BUILD_OBJECT = "buildObject",
                  BUILD_ARRAY = "buildArray",
                  BUILD_UNION = "buildUnion";
