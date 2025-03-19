#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/MutOperations.hpp"

#include "memoir/support/Print.hpp"

namespace llvm::memoir {

map<llvm::Instruction *, MemOIRInst *> *MemOIRInst::llvm_to_memoir = nullptr;

MemOIRInst *MemOIRInst::get(llvm::Instruction &I) {
  if (MemOIRInst::llvm_to_memoir == nullptr) {
    MemOIRInst::llvm_to_memoir = new map<llvm::Instruction *, MemOIRInst *>();
  }
  auto &llvm_to_memoir = *MemOIRInst::llvm_to_memoir;

  auto call_inst = dyn_cast<llvm::CallInst>(&I);
  if (!call_inst) {
    return nullptr;
  }

  if (!FunctionNames::is_memoir_call(*call_inst)) {
    return nullptr;
  }

  auto memoir_enum = FunctionNames::get_memoir_enum(*call_inst);

  MemOIRInst *base = nullptr;

  // Check if there is an existing MemOIRInst.
  auto found = llvm_to_memoir.find(&I);
  if (found != llvm_to_memoir.end()) {
    auto &found_inst = *(found->second);
    base = &found_inst;
  }

  // If the enums don't match, construct a new one in place.
  switch (memoir_enum) {
    default:
      MEMOIR_UNREACHABLE("Unknown MemOIR instruction encountered");
#define HANDLE_INST(ENUM, FUNC, CLASS)                                         \
  case MemOIR_Func::ENUM: {                                                    \
    auto memoir_inst =                                                         \
        base ? new (base) CLASS(*call_inst) : new CLASS(*call_inst);           \
    if (not base) {                                                            \
      llvm_to_memoir[&I] = memoir_inst;                                        \
    }                                                                          \
    return memoir_inst;                                                        \
  }
#include "memoir/ir/Instructions.def"
#define HANDLE_INST(ENUM, FUNC, CLASS)                                         \
  case MemOIR_Func::ENUM: {                                                    \
    auto memoir_inst =                                                         \
        base ? new (base) CLASS(*call_inst) : new CLASS(*call_inst);           \
    if (not base) {                                                            \
      llvm_to_memoir[&I] = memoir_inst;                                        \
    }                                                                          \
    return memoir_inst;                                                        \
  }
#include "memoir/ir/MutOperations.def"
  }

  return nullptr;
}

void MemOIRInst::invalidate() {
  // TODO: Delete all old instructions.
  delete MemOIRInst::llvm_to_memoir;
  MemOIRInst::llvm_to_memoir = nullptr;
}

/*
 * Top-level methods
 */
llvm::CallInst &MemOIRInst::getCallInst() const {
  return this->call_inst;
}

llvm::Value &MemOIRInst::asValue() const {
  return this->call_inst;
}

llvm::Function &MemOIRInst::getCalledFunction() const {
  return MEMOIR_SANITIZE(
      this->getCallInst().getCalledFunction(),
      "MemOIRInst has been corrupted, CallInst is indirect!");
}

llvm::Module *MemOIRInst::getModule() const {
  return this->getCallInst().getModule();
}

llvm::Function *MemOIRInst::getFunction() const {
  return this->getCallInst().getFunction();
}

llvm::BasicBlock *MemOIRInst::getParent() const {
  return this->getCallInst().getParent();
}

MemOIR_Func MemOIRInst::getKind() const {
  return FunctionNames::get_memoir_enum(this->getCallInst());
}

bool MemOIRInst::is_mutator(MemOIRInst &I) {
  return FunctionNames::is_mutator(I.getCalledFunction());
}

std::ostream &operator<<(std::ostream &os, const MemOIRInst &I) {
  os << I.toString();
  return os;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const MemOIRInst &I) {
  os << I.toString();
  return os;
}

llvm::iterator_range<keyword_iterator> MemOIRInst::keywords() const {
  return llvm::make_range(this->kw_begin(), this->kw_end());
}

keyword_iterator MemOIRInst::kw_begin() const {
  for (auto &arg : this->getCallInst().args()) {
    if (Keyword::is_keyword(arg.get())) {
      return keyword_iterator(&arg);
    }
  }
  return this->kw_end();
}

keyword_iterator MemOIRInst::kw_end() const {
  return keyword_iterator(this->getCallInst().arg_end());
}

bool MemOIRInst::has_keywords() const {
  return this->kw_begin() != this->kw_end();
}

#define KEYWORD(STR, CLASS)                                                    \
  template <>                                                                  \
  std::optional<CLASS> MemOIRInst::get_keyword<CLASS>() const {                \
    for (auto kw : this->keywords()) {                                         \
      if (auto the_kw = try_cast<CLASS>(kw)) {                                 \
        return the_kw;                                                         \
      }                                                                        \
    }                                                                          \
    return {};                                                                 \
  }
#include "memoir/ir/Keywords.def"

} // namespace llvm::memoir
