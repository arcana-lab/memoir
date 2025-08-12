#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "memoir/support/Casting.hpp"
#include "memoir/utility/Metadata.hpp"

#include "folio/Candidate.hpp"
#include "folio/Utilities.hpp"

using namespace llvm::memoir;

namespace folio {

static llvm::cl::opt<bool> disable_use_weakening(
    "disable-use-weakening",
    llvm::cl::desc("Disable weakening uses"),
    llvm::cl::init(true));

llvm::Module &Candidate::module() const {
  return this->front()->module();
}

llvm::Function &Candidate::function() const {
  return MEMOIR_SANITIZE(this->front()->function(),
                         "Object in candidate has no parent function!");
}

Type &Candidate::key_type() const {
  return *this->_key_type;
}

llvm::memoir::Type &Candidate::encoder_type() const {
  auto &data_layout = this->module().getDataLayout();
  auto &size_type = Type::get_size_type(data_layout);
  return AssocType::get(this->key_type(), size_type);
}

llvm::memoir::Type &Candidate::decoder_type() const {
  return SequenceType::get(this->key_type());
}
bool Candidate::build_encoder() const {
  return this->to_encode.size() > 0 or this->to_addkey.size() > 0;
}

bool Candidate::build_decoder() const {
  return this->to_decode.size() > 0;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                              const Candidate &candidate) {
  os << "CANDIDATE: ";
  for (const auto *info : candidate) {
    os << "\n  " << *info;
  }
  return os;
}

} // namespace folio
