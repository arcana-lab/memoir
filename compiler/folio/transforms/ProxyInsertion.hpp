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
  };

  void analyze();

  bool transform();

protected:
  void gather_assoc_objects(llvm::memoir::AllocInst &alloc,
                            llvm::memoir::Type &type,
                            llvm::memoir::vector<unsigned> offsets = {});

  llvm::Module &M;
  llvm::memoir::vector<ObjectInfo> allocations;
};

} // namespace folio
