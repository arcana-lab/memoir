#include "common/analysis/AccessAnalysis.hpp"

namespace llvm::memoir {

std::ostream &AccessSummary::operator<<(std::ostream &os,
                                        const AccessSummary &summary) {
  os << summary.toString();
  return os;
}

std::string ReadSummary::toString(std::string indent) {
  std::stringstream sstream;
  if (this->isMust()) {
    sstream << "(must read: " << std::endl
            << indent << "  LLVM: " << this->call_inst << std::endl
            << indent
            << "  field: " << this->getField().toString(indent + "         ")
            << std::endl
            << indent << ")";
  } else {
    sstream << std::endl
            << indent << "(read: " << std::endl
            << indent
            << "  field: " << this->getField().toString(indent + "         ")
            << std::endl
            << indent << ")";
  }

  return sstream.str();
}

std::string WriteSummary::toString(std::string indent) {
  std::stringstream sstream;
  if (this->isMust()) {
    sstream << "(must write: " << std::endl
            << indent << "  LLVM: " << this->call_inst << std::endl
            << indent
            << "  field: " << this->getField().toString(indent + "         ")
            << std::endl
            << indent << "  value written: " << this->getValueWritten()
            << std::endl
            << ")";
  } else {
    sstream << std::endl
            << indent << "(write: " << std::endl
            << indent
            << "  field: " << this->getField().toString(indent + "         ")
            << std::endl
            << indent << "  value written: " << this->getValueWritten()
            << std::endl
            << indent << ")";
  }

  return sstream.str();
}

std::string MayReadSummary::toString(std::string indent) {
  std::stringstream sstream;
  sstream << "(may read: " << std::endl
          << indent << "  LLVM: " << this->call_inst << std::endl;
  for (auto iter = this->begin(); iter != this->end(); ++iter) {
    auto read_summary = *iter;
    sstream << indent << read_summary->toString(indent + "  ") << std::endl;
  }
  sstream << indent << ")";

  return sstream.str();
}

std::string MayWriteSummary::toString(std::string indent) {
  std::stringstream sstream;
  sstream << "(may write: " << std::endl
          << indent << "  LLVM: " << this->call_inst << std::endl;
  for (auto iter = this->begin(); iter != this->end(); ++iter) {
    auto write_summary = *iter;
    sstream << indent << write_summary->toString(indent + "  ") << std::endl;
  }
  sstream << indent << ")";

  return sstream.str();
}

} // namespace llvm::memoir
