#include "common/analysis/AccessAnalysis.hpp"
#include "common/utility/Metadata.hpp"

namespace llvm::memoir {

/*
 * Top-level initialization.
 */
AccessAnalysis::AccessAnalysis(Module &M) : M(M) {
  /*
   * Initialize the analysis.
   */
  initialize();

  /*
   * Perform the analysis.
   */
  analyze();
}

void AccessAnalysis::initialize() {
  errs() << "AccessAnalysis: initialize.\n";

  /*
   * Perform all needed initialization for the AccessAnalysis pass.
   */
  return;
}

/*
 * Top-level analysis.
 */
void AccessAnalysis::analyze() {
  errs() << "AccessAnalysis: analyzing.\n";

  /*
   * Analyze all of the objects present in the program, both allocations and
   * nested objects.
   */
  analyzeObjects();

  /*
   * Analyze the fields of the objects present in the program.
   */
  analyzeFields();

  /*
   * Reconcile the objects that are accessed through references.
   */
  reconcileReferences();

  /*
   * Analyze the accesses within the program.
   */
  analyzeAccesses();

  errs() << "AccessAnalysis: done.\n";

  return;
}

/*
 * Utility Functions
 */
bool AccessAnalysis::isRead(MemOIR_Func func_enum) {
  switch (func_enum) {
    case MemOIR_Func::READ_INTEGER:
    case MemOIR_Func::READ_UINT64:
    case MemOIR_Func::READ_UINT32:
    case MemOIR_Func::READ_UINT16:
    case MemOIR_Func::READ_UINT8:
    case MemOIR_Func::READ_INT64:
    case MemOIR_Func::READ_INT32:
    case MemOIR_Func::READ_INT16:
    case MemOIR_Func::READ_INT8:
    case MemOIR_Func::READ_FLOAT:
    case MemOIR_Func::READ_DOUBLE:
    case MemOIR_Func::READ_REFERENCE:
    case MemOIR_Func::READ_STRUCT:
    case MemOIR_Func::READ_TENSOR:
      return true;
    default:
      return false;
  }
}

bool AccessAnalysis::isWrite(MemOIR_Func func_enum) {
  switch (func_enum) {
    case MemOIR_Func::WRITE_INTEGER:
    case MemOIR_Func::WRITE_UINT64:
    case MemOIR_Func::WRITE_UINT32:
    case MemOIR_Func::WRITE_UINT16:
    case MemOIR_Func::WRITE_UINT8:
    case MemOIR_Func::WRITE_INT64:
    case MemOIR_Func::WRITE_INT32:
    case MemOIR_Func::WRITE_INT16:
    case MemOIR_Func::WRITE_INT8:
    case MemOIR_Func::WRITE_FLOAT:
    case MemOIR_Func::WRITE_DOUBLE:
    case MemOIR_Func::WRITE_REFERENCE:
      return true;
    default:
      return false;
  }
}

bool AccessAnalysis::isAccess(MemOIR_Func func_enum) {
  switch (func_enum) {
    case MemOIR_Func::READ_INTEGER:
    case MemOIR_Func::READ_UINT64:
    case MemOIR_Func::READ_UINT32:
    case MemOIR_Func::READ_UINT16:
    case MemOIR_Func::READ_UINT8:
    case MemOIR_Func::READ_INT64:
    case MemOIR_Func::READ_INT32:
    case MemOIR_Func::READ_INT16:
    case MemOIR_Func::READ_INT8:
    case MemOIR_Func::READ_FLOAT:
    case MemOIR_Func::READ_DOUBLE:
    case MemOIR_Func::READ_REFERENCE:
    case MemOIR_Func::READ_STRUCT:
    case MemOIR_Func::READ_TENSOR:
    case MemOIR_Func::WRITE_INTEGER:
    case MemOIR_Func::WRITE_UINT64:
    case MemOIR_Func::WRITE_UINT32:
    case MemOIR_Func::WRITE_UINT16:
    case MemOIR_Func::WRITE_UINT8:
    case MemOIR_Func::WRITE_INT64:
    case MemOIR_Func::WRITE_INT32:
    case MemOIR_Func::WRITE_INT16:
    case MemOIR_Func::WRITE_INT8:
    case MemOIR_Func::WRITE_FLOAT:
    case MemOIR_Func::WRITE_DOUBLE:
    case MemOIR_Func::WRITE_REFERENCE:
      return true;
    default:
      return false;
  }
}

/*
 * Management
 */
AccessAnalysis &AccessAnalysis::get(Module &M) {
  auto found_analysis = AccessAnalysis::analyses.find(&M);
  if (found_analysis != AccessAnalysis::analyses.end()) {
    return *(found_analysis->second);
  }

  auto new_analysis = new AccessAnalysis(M);
  AccessAnalysis::analyses[&M] = new_analysis;
  return *new_analysis;
}

void AccessAnalysis::invalidate(Module &M) {
  auto found_analysis = AccessAnalysis::analyses.find(&M);
  if (found_analysis != AccessAnalysis::analyses.end()) {
    auto &analysis = *(found_analysis->second);
    analysis.invalidate();
  }

  return;
}

void AccessAnalysis::invalidate() {
  this->access_summaries.clear();
  this->field_summaries.clear();

  return;
}

map<llvm::Module *, AccessAnalysis *> AccessAnalysis::analyses = {};

} // namespace llvm::memoir
