#include "memoir/ir/Instructions.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/InternalDatatypes.hpp"

namespace folio {

struct ProxyInsertion {
public:
  ProxyInsertion(llvm::Module &M);

  struct ObjectInfo {
    llvm::memoir::AllocInst *allocation;
    llvm::memoir::vector<unsigned> offsets;

    ObjectInfo(llvm::memoir::AllocInst &alloc, llvm::ArrayRef<unsigned> offsets)
      : allocation(&alloc),
        offsets(offsets.begin(), offsets.end()) {}
    ObjectInfo(llvm::memoir::AllocInst &alloc) : ObjectInfo(alloc, {}) {}

    friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                         const ObjectInfo &info) {
      os << "(" << *info.allocation << ")";
      for (auto offset : info.offsets) {
        if (offset == -1) {
          os << "[*]";
        } else {
          os << "." << std::to_string(offset);
        }
      }

      return os;
    }
  };

  void analyze();

  bool transform();

protected:
  void gather_assoc_objects(llvm::memoir::vector<ObjectInfo> &allocations,
                            llvm::memoir::AllocInst &alloc,
                            llvm::memoir::Type &type,
                            llvm::memoir::vector<unsigned> offsets = {});

  llvm::Module &M;
  llvm::memoir::vector<llvm::memoir::list<ObjectInfo>> candidates;
};

} // namespace folio
