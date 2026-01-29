#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"

#include "memoir/ir/Builder.hpp"
#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/Types.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/DataTypes.hpp"

#include "AccessCounter.hpp"

using namespace memoir;

static llvm::cl::opt<bool> count_per_operation(
    "memoir-profile-per-op",
    llvm::cl::desc("Count occurences of each individual access operation"));

using Count = uint64_t;

llvm::Type *counter_type = nullptr;

static llvm::GlobalVariable *create_counter(llvm::Module &M,
                                            llvm::StringRef name) {

  auto &context = M.getContext();
  auto *zero_init = llvm::ConstantInt::get(counter_type, 0);

  auto *global =
      new llvm::GlobalVariable(M,
                               counter_type,
                               /* constant? */ false,
                               llvm::GlobalValue::LinkageTypes::InternalLinkage,
                               zero_init,
                               "memoir_counter__" + name.str());
  MEMOIR_ASSERT(global, "Failed to create global!");

  return global;
}

static void increment(llvm::IRBuilder<> &builder,
                      llvm::GlobalVariable *global,
                      llvm::Value *amount) {
  auto *load = builder.CreateLoad(counter_type, global);
  auto *add = builder.CreateAdd(load, amount);
  builder.CreateStore(add, global);
}

static void increment(llvm::Instruction *inst,
                      llvm::GlobalVariable *global,
                      llvm::Value *amount) {
  llvm::IRBuilder<> builder(inst);
  increment(builder, global, amount);
}

static void increment(llvm::Instruction *inst,
                      llvm::GlobalVariable *global,
                      unsigned amount = 1) {
  if (amount == 0)
    return;

  increment(inst, global, llvm::ConstantInt::get(counter_type, amount));
}

struct Counter {
  Counter(llvm::Module &module, std::initializer_list<std::string> names) {
    for (const auto &name : names)
      this->counters[name] = create_counter(module, name);
  }

  virtual ~Counter() = default;

  virtual void count(AccessInst &access) = 0;

  llvm::GlobalVariable *global(llvm::StringRef name) {
    return this->counters[name];
  }

  llvm::StringMap<llvm::GlobalVariable *> counters;
};

struct SparseDenseCounter : public Counter {
public:
  SparseDenseCounter(llvm::Module &module)
    : Counter(module, { "sparse", "dense" }) {}

  void count(AccessInst &access) override {
    auto dense = 0, sparse = 0;

    // Get the type of the innermost object.
    auto *type = &access.getObjectType();
    for (auto *index : access.indices()) {
      if (auto *collection_type = dyn_cast<CollectionType>(type)) {
        if (isa<SequenceType>(collection_type)) {
          ++dense;
        } else if (isa<AssocType>(collection_type)) {
          auto selection = collection_type->get_selection();
          if (selection
              and (selection.value() == "bitset"
                   or selection.value() == "bitmap")) {
            ++dense;
          } else {
            ++sparse;
          }
        }

        // Get the nested type.
        type = &collection_type->getElementType();

      } else if (auto *tuple_type = dyn_cast<TupleType>(type)) {
        auto &index_constant =
            MEMOIR_SANITIZE(dyn_cast<llvm::ConstantInt>(index),
                            "Struct field index is not constant.\n  ",
                            *index,
                            " in ",
                            access);
        auto index_value = index_constant.getZExtValue();

        type = &tuple_type->getFieldType(index_value);

      } else {
        break;
      }
    }

    auto *dense_glob = this->global("dense");
    increment(&access.getCallInst(), dense_glob, dense);

    auto *sparse_glob = this->global("sparse");
    increment(&access.getCallInst(), sparse_glob, sparse);
  }
};

struct TypeCounter : public Counter {
public:
  TypeCounter(llvm::Module &module)
    : Counter(
          module,
          { "seq_read",   "seq_get",    "seq_copy",   "seq_size",   "seq_write",
            "seq_insert", "seq_remove", "seq_has",    "seq_fold",

            "map_read",   "map_get",    "map_copy",   "map_size",   "map_write",
            "map_insert", "map_remove", "map_has",    "map_fold",

            "set_copy",   "set_size",   "set_insert", "set_remove", "set_has",
            "set_fold" }) {}

  llvm::GlobalVariable *op_global(Type *type, AccessInst &access) {
    if (isa<ReadInst>(&access))
      return this->op_global(type, "read");
    if (isa<GetInst>(&access))
      return this->op_global(type, "get");
    if (isa<CopyInst>(&access))
      return this->op_global(type, "copy");
    if (isa<SizeInst>(&access))
      return this->op_global(type, "size");
    if (isa<WriteInst>(&access))
      return this->op_global(type, "write");
    if (isa<InsertInst>(&access))
      return this->op_global(type, "insert");
    if (isa<RemoveInst>(&access))
      return this->op_global(type, "remove");
    if (isa<HasInst>(&access))
      return this->op_global(type, "has");
    if (isa<FoldInst>(&access))
      return this->op_global(type, "fold");
    return NULL;
  }

  llvm::GlobalVariable *op_global(Type *type, std::string access_name) {
    if (isa<SequenceType>(type))
      return this->global("seq_" + access_name);

    if (auto *assoc = dyn_cast<AssocType>(type)) {
      if (isa<VoidType>(assoc->getValueType()))
        return this->global("set_" + access_name);

      return this->global("map_" + access_name);
    }

    return NULL;
  }

  void count(AccessInst &access) override {

    // Is this an aggregate operation?
    bool aggregate =
        isa<ClearInst>(&access) or isa<CopyInst>(&access)
        or isa<SizeInst>(&access) or isa<FoldInst>(&access)
        or (isa<InsertInst>(&access) and access.get_keyword<InputKeyword>());

    // Get the type of the innermost object.
    auto *type = &access.getObjectType();
    auto ie = access.indices_end();
    for (auto it = access.indices_begin(); it != ie; ++it) {
      auto *index = *it;

      // If this is the last index and this is not an aggregate operation, stop
      // counting reads!
      if (not aggregate and (std::next(it) == ie))
        break;

      if (auto *collection_type = dyn_cast<CollectionType>(type)) {
        // Update the local op counter.
        if (auto *global = this->op_global(collection_type, "read"))
          increment(&access.getCallInst(), global);

        // Get the nested type.
        type = &collection_type->getElementType();

      } else if (auto *tuple_type = dyn_cast<TupleType>(type)) {
        // Get the nested type.
        auto &index_constant =
            MEMOIR_SANITIZE(dyn_cast<llvm::ConstantInt>(index),
                            "Struct field index is not constant.\n  ",
                            *index,
                            " in ",
                            access);
        auto index_value = index_constant.getZExtValue();
        type = &tuple_type->getFieldType(index_value);

      } else {
        return;
      }
    }

    // Get the op local.
    auto *global = this->op_global(type, access);
    if (not global)
      return;

    // If this is an insert-input operation, we need to count the number of
    // elements inserted.
    auto input_kw = access.get_keyword<InputKeyword>();
    if (isa<InsertInst>(&access) and input_kw) {
      // Add the size of the input collection.
      MemOIRBuilder builder(&access.getCallInst());

      SmallVector<llvm::Value *> args(input_kw->indices_begin(),
                                      input_kw->indices_end());
      auto *size_inst =
          builder.CreateSizeInst(&input_kw->getInput(), args, "prof.");
      auto *size = &size_inst->asValue();

      increment(builder, global, size);

      return;
    }

    // Count the outermost operation.
    increment(&access.getCallInst(), global);
  }
};

static void create_report(llvm::Module &module,
                          llvm::ArrayRef<Counter *> counters) {
  auto &context = module.getContext();

  // Create an empty function.
  auto *void_type = llvm::Type::getVoidTy(context);
  auto *func_type = llvm::FunctionType::get(void_type, /* variadic? */ false);

  auto *func =
      llvm::Function::Create(func_type,
                             llvm::GlobalValue::LinkageTypes::InternalLinkage,
                             "memoir_report_accesses",
                             module);

  auto *block = llvm::BasicBlock::Create(context, "", func);

  MemOIRBuilder builder(block);

  // Create the print format string.
  bool first = true;
  std::string header_str = "";
  std::string data_str = "";
  for (auto *counter : counters) {
    for (const auto &[name, _info] : counter->counters) {
      if (first) {
        first = false;
      } else {
        header_str += ",";
        data_str += ",";
      }
      header_str += name;
      data_str += "%lu";
    }
  }

  std::string format_str = header_str + "\n" + data_str + "\n";

  Vector<llvm::Value *> args = {};
  for (auto *counter : counters) {
    for (const auto &[_name, global] : counter->counters) {
      auto *load = builder.CreateLoad(counter_type, global);
      args.push_back(load);
    }
  }

  builder.CreateErrorf(format_str, args);

  builder.CreateRetVoid();

  // Record the report function in the module's destructor.
  llvm::appendToGlobalDtors(module, func, /* priority */ 10);
}

static bool transform(llvm::Module &module) {
  // Create the counters.
  Vector<Counter *> counters = { new SparseDenseCounter(module) };
  if (count_per_operation)
    counters.push_back(new TypeCounter(module));

  // Create the global variable counters.
  auto *sparse_counter = create_counter(module, "memoir_sparse_access_counter");
  auto *dense_counter = create_counter(module, "memoir_dense_access_counter");

  // Find all accesses that need to be marked.
  for (auto &func : module) {
    for (auto &block : func) {

      // Count the local accesses.
      Vector<AccessInst *> accesses = {};
      for (auto &inst : block)
        if (auto *access = into<AccessInst>(&inst))
          accesses.push_back(access);

      for (auto *access : accesses)
        for (auto *counter : counters)
          counter->count(*access);
    }
  }

  // Create a report function to call at destruction.
  create_report(module, counters);

  // Cleanup.
  for (auto *counter : counters)
    delete counter;

  return true;
}

AccessCounter::AccessCounter(llvm::Module &module) {
  auto &context = module.getContext();
  counter_type = llvm::Type::getInt64Ty(context);

  transform(module);
}
