#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/IR/DiagnosticInfo.h"

#include "DataEnumeration.hpp"

namespace memoir {

void DataEnumeration::remark(const llvm::Instruction *inst,
                             llvm::StringRef remark_name,
                             llvm::StringRef message) {
  llvm::OptimizationRemarkEmitter emitter(inst->getFunction());
  llvm::OptimizationRemark remark("memoir-ade", remark_name, inst);
  remark << message;
  emitter.emit(remark);
}

void DataEnumeration::remark(const llvm::Function *func,
                             llvm::StringRef remark_name,
                             llvm::StringRef message) {
  llvm::OptimizationRemarkEmitter emitter(func);
  llvm::OptimizationRemark remark("memoir-ade", remark_name, func);
  remark << message;
  emitter.emit(remark);
}

} // namespace memoir
