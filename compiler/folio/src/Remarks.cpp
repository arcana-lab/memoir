#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/IR/DiagnosticInfo.h"

#include "folio/ProxyInsertion.hpp"

namespace folio {

void ProxyInsertion::remark(const llvm::Instruction *inst,
                            llvm::StringRef remark_name,
                            llvm::StringRef message) {
  llvm::OptimizationRemarkEmitter emitter(inst->getFunction());
  llvm::OptimizationRemark remark("memoir-ade", remark_name, inst);
  remark << message;
  emitter.emit(remark);
}

void ProxyInsertion::remark(const llvm::Function *func,
                            llvm::StringRef remark_name,
                            llvm::StringRef message) {
  llvm::OptimizationRemarkEmitter emitter(func);
  llvm::OptimizationRemark remark("memoir-ade", remark_name, func);
  remark << message;
  emitter.emit(remark);
}

} // namespace folio
