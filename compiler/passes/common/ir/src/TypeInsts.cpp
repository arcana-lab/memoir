#include "Instructions.hpp"

/*
 * Implementation of the MemOIR Type Instructions.
 *
 * Author(s): Tommy McMichen
 * Created: December 13, 2022
 */

/*
 * IntegerTypeInst implementation
 */
Type &IntegerTypeInst::getType() const {
  return;
}

std::string IntegerTypeInst::toString(std::string indent = "") const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "IntegerTypeInst: " + llvm_str;

  return str;
}

/*
 * FloatType implementation
 */
Type &FloatTypeInst::getType() const {
  return;
}

std::string FloatTypeInst::toString(std::string indent = "") const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "FloatTypeInst: " + llvm_str;

  return str;
}

/*
 * DoubleType implementation
 */
Type &DoubleTypeInst::getType() const {
  return;
}

std::string DoubleTypeInst::toString(std::string indent = "") const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "DoubleTypeInst: " + llvm_str;

  return str;
}

/*
 * PointerType implementation
 */
Type &PointerTypeInst::getType() const {
  return;
}

std::string PointerTypeInst::toString(std::string indent = "") const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "PointerTypeInst: " + llvm_str;

  return str;
}

/*
 * ReferenceType implementation
 */
Type &ReferenceTypeInst::getType() const {
  return;
}

std::string ReferenceTypeInst::toString(std::string indent = "") const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "ReferenceTypeInst: " + llvm_str;

  return str;
}

/*
 * DefineStructType implementation
 */
Type &DefineStructTypeInst::getType() const {
  return;
}

std::string DefineStructTypeInst::toString(std::string indent = "") const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "DefineStructTypeInst: " + llvm_str;

  return str;
}

/*
 * StructType implementation
 */
Type &StructTypeInst::getType() const {
  return;
}

std::string StructTypeInst::toString(std::string indent = "") const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "StructTypeInst: " + llvm_str;

  return str;
}

/*
 * StaticTensorType implementation
 */
Type &StaticTensorTypeInst::getType() const {
  return;
}

std::string StaticTensorTypeInst::toString(std::string indent = "") const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "StaticTensorTypeInst: " + llvm_str;

  return str;
}

/*
 * TensorType implementation
 */
Type &TensorTypeInst::getType() const {
  return;
}

std::string TensorTypeInst::toString(std::string indent = "") const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "TensorTypeInst: " + llvm_str;

  return str;
}

/*
 * AssocArrayType implementation
 */
Type &AssocArrayTypeInst::getType() const {
  return;
}

std::string AssocArrayTypeInst::toString(std::string indent = "") const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "AssocArrayTypeInst: " + llvm_str;

  return str;
}

/*
 * SequenceType implementation
 */

std::string SequenceTypeInst::toString(std::string indent = "") const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "SequenceTypeInst: " + llvm_str;

  return str;
}

Type &SequenceTypeInst::getType() const {
  return;
}
