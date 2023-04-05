#include <llvm/IR/DerivedTypes.h>
#include "memoir/ir/Types.hpp"

namespace object_lowering {
    class NativeTypeConverter {
    public:
        NativeTypeConverter(llvm::Module &M);
        llvm::Type *getLLVMRepresentation(llvm::memoir::Type* type);
    private:
        llvm::Module &M;
        std::map<llvm::memoir::Type*, llvm::Type*> cache;
        llvm::StructType* getLLVMRepresentation(llvm::memoir::StructType* type);
    };
}
