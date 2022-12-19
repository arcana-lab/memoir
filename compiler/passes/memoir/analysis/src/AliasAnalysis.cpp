#include "memoir/analysis/AliasAnalysis.hpp"

namespace llvm::memoir {

// AliasInfo AliasAnalysis::aliases(FieldSummary &this_field,
//                                  FieldSummary &other_field) {
//   auto &this_points_to = this_field.pointsTo();
//   auto &other_points_to = other_field.pointsTo();

//   if (&this_points_to != &other_points_to) {
//     return AliasInfo::NONE;
//   }

//   auto type_code = this_points_to.getTypeCode();
//   switch (type_code) {
//     case TypeCode::StructTy: {
//       auto &this_struct_field = static_cast<StructFieldSummary
//       &>(this_field); auto &other_struct_field =
//       static_cast<StructFieldSummary &>(other_field);

//       /*
//        * Check that their index is equal.
//        */
//       if (this_struct_field.getIndex() == other_struct_field.getIndex()) {
//         return AliasInfo::MUST;
//       }

//       break;
//     }
//     case TypeCode::TensorTy: {
//       auto &this_tensor_element =
//           static_cast<TensorElementSummary &>(this_field);
//       auto &other_tensor_element =
//           static_cast<TensorElementSummary &>(other_field);

//       /*
//        * Check all of the dimension indices.
//        *  - If any of the indices do not match, these tensor elements can NOT
//        *    alias.
//        *  - If the indices all match, these tensor elements MUST alias.
//        *  - Otherwise, if the indices are not constant, they MAY alias.
//        */
//       bool all_indices_match = true;
//       auto num_dimensions = this_tensor_element.getNumberOfDimensions();
//       for (auto dim_index = 0; dim_index < num_dimensions; dim_index++) {
//         auto &this_index = this_tensor_element.getIndex(dim_index);
//         auto &other_index = other_tensor_element.getIndex(dim_index);

//         /*
//          * If both indices are constant, check if they match.
//          */
//         auto this_index_const = dyn_cast<ConstantInt>(&this_index);
//         auto other_index_const = dyn_cast<ConstantInt>(&other_index);
//         if (this_index_const && other_index_const) {
//           auto this_index_integer = this_index_const->getZExtValue();
//           auto other_index_integer = other_index_const->getZExtValue();
//           if (this_index_integer != other_index_integer) {
//             return AliasInfo::NONE;
//           } else {
//             all_indices_match &= true;
//           }
//         }
//       }

//       /*
//        * If all of the indices match then the fields must alias
//        */
//       if (all_indices_match) {
//         return AliasInfo::MUST;
//       }

//       /*
//        * If the indices cannot be shown to all match or definitely NOT match,
//        * return MAY alias
//        */
//       return AliasInfo::MAY;

//       break;
//     }
//     default: {
//       break;
//     }
//   }

//   return AliasInfo::NONE;
// }
} // namespace llvm::memoir
