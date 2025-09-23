#ifndef FOLIO_TRANSFORMS_OBJECTINFO_H
#define FOLIO_TRANSFORMS_OBJECTINFO_H

#include "llvm/IR/Argument.h"
#include "llvm/Transforms/Utils/ValueMapper.h"

#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/Object.hpp"
#include "memoir/ir/Types.hpp"
#include "memoir/support/DataTypes.hpp"

#include "folio/Context.hpp"
#include "folio/NestedObject.hpp"
#include "folio/Utilities.hpp"

namespace folio {

using memoir::Object;
using memoir::Offset;
using memoir::Offsets;
using memoir::OffsetsRef;

enum ObjectInfoKind { OIK_Base, OIK_Arg };

struct LocalInfo {
  Set<Object> redefinitions;
  Set<llvm::Value *> encoded;
  Set<llvm::Use *> to_encode;
  Set<llvm::Use *> to_addkey;
};

struct ObjectInfo : public Object {
protected:
  /** The kind of ObjectInfo */
  ObjectInfoKind _kind;

  /** Per-function information about the object. */
  Map<llvm::Function *, LocalInfo> _info;

  void decode(llvm::Function &function, llvm::Value &value);
  void encode(llvm::Function &function, llvm::Use &use);
  void addkey(llvm::Function &function, llvm::Use &use);

public:
  /** Query the kind of this ObjectInfo */
  const ObjectInfoKind &kind() const;

  /** The base value of this object */
  using Object::value;

  /** The offset of this object ion the base value */
  using Object::offsets;

  /** The type of the object */
  using Object::type;

  Map<llvm::Function *, LocalInfo> &info();
  const Map<llvm::Function *, LocalInfo> &info() const;

  LocalInfo &local(llvm::Function &function);
  const LocalInfo &local(llvm::Function &function) const;

  Set<Object> &redefinitions(llvm::Function &function);
  const Set<Object> &redefinitions(llvm::Function &function) const;

  Set<llvm::Value *> &encoded(llvm::Function &function);
  const Set<llvm::Value *> &encoded(llvm::Function &function) const;

  Set<llvm::Use *> &to_encode(llvm::Function &function);
  const Set<llvm::Use *> &to_encode(llvm::Function &function) const;

  Set<llvm::Use *> &to_addkey(llvm::Function &function);
  const Set<llvm::Use *> &to_addkey(llvm::Function &function) const;

  /** Check if this nested object is a propagator. */
  bool is_propagator() const;

protected:
  void gather_redefinitions(const Object &obj);
  void gather_redefinitions();

  void gather_uses_to_proxy(const Object &obj);
  void gather_uses_to_propagate(const Object &obj);

public:
  /** Analyze the object, populating local info */
  void analyze();

  /**
   * Check if the given value is a redefinition of the object.
   */
  bool is_redefinition(llvm::Value &V) const;

  /**
   * Update the analysis results for this object info.
   * @param old_func, the old function.
   * @param new_func, the new function.
   * @param vmap, a map from original values to cloned values.
   * @param delete_old, if we should delete information about the old
   * function.
   */
  void update(llvm::Function &old_func,
              llvm::Function &new_func,
              llvm::ValueToValueMapTy &vmap,
              bool delete_old = false);

  /** Constructor */
  ObjectInfo(ObjectInfoKind kind,
             llvm::Value &value,
             llvm::ArrayRef<unsigned> offsets = {})
    : Object(value, offsets),
      _kind(kind),
      _info{} {}
};

struct BaseObjectInfo : public ObjectInfo {
  /** Get the the base value of the object as an allocation. */
  memoir::AllocInst &allocation() const;

  BaseObjectInfo(memoir::AllocInst &alloc, OffsetsRef offsets = {})
    : ObjectInfo(ObjectInfoKind::OIK_Base, alloc.asValue(), offsets) {}

  static bool classof(const ObjectInfo *obj) {
    return obj->kind() == ObjectInfoKind::OIK_Base;
  }
};

struct ArgObjectInfo : public ObjectInfo {
protected:
  Map<llvm::CallBase *, ObjectInfo *> _incoming;

public:
  /** Get the base argument of this abstract object */
  llvm::Argument &argument() const;

  /** Get the context-sensitive incoming objects */
  Map<llvm::CallBase *, ObjectInfo *> &incoming();
  const Map<llvm::CallBase *, ObjectInfo *> &incoming() const;

  /** Get the incoming object along this call edge, if it exists */
  ObjectInfo *incoming(llvm::CallBase &call) const;

  /** Add an incoming object along a call edge. */
  void incoming(llvm::CallBase &call, ObjectInfo &obj);

  ArgObjectInfo(llvm::Argument &arg, OffsetsRef offsets = {})
    : ObjectInfo(ObjectInfoKind::OIK_Arg, arg, offsets),
      _incoming{} {}

  static bool classof(const ObjectInfo *obj) {
    return obj->kind() == ObjectInfoKind::OIK_Arg;
  }
};

} // namespace folio

#endif
