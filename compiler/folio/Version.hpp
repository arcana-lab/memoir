#ifndef FOLIO_TRANSFORMS_VERSION_H
#define FOLIO_TRANSFORMS_VERSION_H

namespace folio {

struct Version : public llvm::memoir::Vector<Candidate *> {
  using Base = llvm::memoir::Vector<Candidate *>;

  llvm::Function *func;
  llvm::CallBase *call;
  llvm::memoir::Map<llvm::CallBase *, llvm::memoir::Vector<Candidate *>>
      callers;
  llvm::memoir::Vector<llvm::GlobalVariable *> encoder_args, decoder_args;

  Version(llvm::Function *func, llvm::CallBase *call, size_t num_args)
    : Base(num_args, NULL),
      func(func),
      call(call),
      callers{},
      encoder_args(num_args, NULL),
      decoder_args(num_args, NULL) {}

  void add_caller(llvm::CallBase *call,
                  const llvm::memoir::Vector<Candidate *> &alias) {
    if (call) {
      this->callers[call] = alias;
    }
  }

  bool is_polymorphic(size_t index) const {
    Candidate *base = this->at(index);

    for (const auto &[call, args] : this->callers) {
      if (base != args.at(index)) {
        return true;
      }
    }

    return false;
  }
};

} // namespace folio

#endif
