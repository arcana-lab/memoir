#include <iostream>
#include <math.h>
#include <string.h>

#include "object_ir.h"

using namespace objectir;

enum Kind {
  Val, // 0
  Var, // 1
  Add, // 2
  Mul, // 3
};

Type *Expr = nameObjectType(
    "Expr",         // merged subclasses
    4,              // # fields
    getUInt8Type(), // kind
    getInt64Type(), // name/value
    getPointerType(getNamedType("Expr")), // left
    getPointerType(getNamedType("Expr"))  // right
);

/*
class Expr {
public:
  Kind kind;
  Expr(Kind k) {
    this->kind = k;
  }
};
*/

/*
class ValExpr : public Expr {
public:
  long value;
  ValExpr(long i) : Expr(Val) {
    this->value = i;
  }
};
*/

Object *new_ValExpr(int64_t i) {
  setReturnType(Expr);

  auto newValExpr = buildObject(Expr);
  auto newValExpr_kindFld = getObjectField(newValExpr, 0);
  auto newValExpr_valueFld = getObjectField(newValExpr, 1);
  writeUInt8(newValExpr_kindFld, (uint8_t)Kind::Val);
  writeInt64(newValExpr_valueFld, i);

  return newValExpr;
}

/*
class VarExpr : public Expr {
public:
  long name;
  VarExpr(long n) : Expr(Var) {
    this->name = n;
  }
};
*/

Object *new_VarExpr(int64_t n) {
  setReturnType(Expr);

  auto newVarExpr = buildObject(Expr);
  auto newVarExpr_kindFld = getObjectField(newVarExpr, 0);
  auto newVarExpr_nameFld = getObjectField(newVarExpr, 1);
  writeUInt8(newVarExpr_kindFld, (uint8_t)Kind::Var);
  writeInt64(newVarExpr_nameFld, n);

  return newVarExpr;
}

/*
class AddExpr : public Expr {
public:
  const Expr *left;
  const Expr *right;
  AddExpr(const Expr *e1, const Expr *e2) : Expr(Add) {
    this->left = e1;
    this->right = e2;
  }
};
*/

Object *new_AddExpr(Object *left, Object *right) {
  setReturnType(Expr);
  assertType(Expr, left);
  assertType(Expr, right);

  auto newAddExpr = buildObject(Expr);
  auto newAddExpr_kindFld = getObjectField(newAddExpr, 0);
  auto newAddExpr_leftFld = getObjectField(newAddExpr, 2);
  auto newAddExpr_rightFld = getObjectField(newAddExpr, 3);
  writeUInt8(newAddExpr_kindFld, (uint8_t)Kind::Add);
  writePointer(newAddExpr_leftFld, left);
  writePointer(newAddExpr_rightFld, right);

  return newAddExpr;
}

/*
class MulExpr : public Expr {
public:
  const Expr *left;
  const Expr *right;
  MulExpr(const Expr *e1, const Expr *e2) : Expr(Mul) {
    this->left = e1;
    this->right = e2;
  }
};
*/

Object *new_MulExpr(Object *left, Object *right) {
  setReturnType(Expr);
  assertType(Expr, left);
  assertType(Expr, right);

  auto newMulExpr = buildObject(Expr);
  auto newMulExpr_kindFld = getObjectField(newMulExpr, 0);
  auto newMulExpr_leftFld = getObjectField(newMulExpr, 2);
  auto newMulExpr_rightFld = getObjectField(newMulExpr, 3);
  writeUInt8(newMulExpr_kindFld, (uint8_t)Kind::Mul);
  writePointer(newMulExpr_leftFld, left);
  writePointer(newMulExpr_rightFld, right);

  return newMulExpr;
}

/*
static const Expr *mk_expr(long n, long v) {
  if (n == 0) {
    if (v == 0)
      return new VarExpr(1);
    else
      return new ValExpr(v);
  } else {
    return new AddExpr(mk_expr(n - 1, v + 1),
                       mk_expr(n - 1, v == 0 ? 0 : v - 1));
  }
}
*/

static Object *mk_expr(int64_t n, int64_t v) {
  setReturnType(Expr);

  if (n == 0) {
    if (v == 0) {
      return new_VarExpr(1);
    } else {
      return new_ValExpr(v);
    }
  } else {
    return new_AddExpr(mk_expr(n - 1, v + 1),
                       mk_expr(n - 1, v == 0 ? 0 : v - 1));
  }
}

static Object *append_add(Object *e1, Object *e2) {
  std::cerr << "append_add\n";
  std::cerr << e1 << "\n";
  std::cerr << e2 << "\n";
  if (e1) {
    std::cerr << e1->toString() << "\n";
  }
  if (e2) {
    std::cerr << e2->toString() << "\n";
  }

  setReturnType(Expr);
  assertType(Expr, e1);
  assertType(Expr, e2);

  Field *e1_kindFld = getObjectField(e1, 0);
  Kind e1_kind = (Kind)readUInt8(e1_kindFld);

  if (e1_kind == Kind::Add) {
    // const AddExpr *x = (AddExpr *)e1;
    auto x = e1;

    Field *x_leftField = getObjectField(x, 2);
    Field *x_rightField = getObjectField(x, 3);
    Object *x_left = readPointer(x_leftField);
    Object *x_right = readPointer(x_rightField);

    // return new AddExpr(x->left, append_add(x->right,
    // e2));
    return new_AddExpr(x_left, append_add(x_right, e2));
  } else {
    // return new AddExpr(e1, e2);
    return new_AddExpr(e1, e2);
  }
}

static Object *append_mul(Object *e1, Object *e2) {
  setReturnType(Expr);
  assertType(Expr, e1);
  assertType(Expr, e2);

  Field *kindFld = getObjectField(e1, 0);
  Kind kind = (Kind)readUInt8(kindFld);

  if (kind == Kind::Mul) {
    // const MulExpr *x = (MulExpr *)e1;
    Object *x = e1;
    Field *x_leftField = getObjectField(x, 2);
    Field *x_rightField = getObjectField(x, 3);
    Object *x_left = readPointer(x_leftField);
    Object *x_right = readPointer(x_rightField);

    // return new MulExpr(x->left, append_mul(x->right,
    // e2));
    return new_MulExpr(x_left, append_mul(x_right, e2));
  } else {
    // return new MulExpr(e1, e2);
    return new_MulExpr(e1, e2);
  }
}

static Object *reassoc(Object *e) {
  setReturnType(Expr);
  assertType(Expr, e);

  auto e_kindFld = getObjectField(e, 0);
  auto e_kind = (Kind)readUInt8(e_kindFld);

  if (e_kind == Add) {
    // const AddExpr *x = (AddExpr *)e;
    Object *x = e;

    // return append_add(reassoc(x->left),
    // reassoc(x->right));
    Field *x_leftFld = getObjectField(x, 2);
    Field *x_rightFld = getObjectField(x, 3);
    Object *x_left = readPointer(x_leftFld);
    Object *x_right = readPointer(x_rightFld);

    return append_add(reassoc(x_left), reassoc(x_right));

  } else if (e_kind == Mul) {
    // const MulExpr *x = (MulExpr *)e;
    Object *x = e;

    // return append_mul(reassoc(x->left),
    // reassoc(x->right));
    Field *x_leftFld = getObjectField(x, 2);
    Field *x_rightFld = getObjectField(x, 3);
    Object *x_left = readPointer(x_leftFld);
    Object *x_right = readPointer(x_rightFld);

    return append_mul(reassoc(x_left), reassoc(x_right));

  } else {
    return e;
  }
}

static Object *const_folding(Object *e) {
  setReturnType(Expr);
  assertType(Expr, e);

  Field *kindFld = getObjectField(e, 0);
  Field *valFld = getObjectField(e, 1);
  Field *leftFld = getObjectField(e, 2);
  Field *rightFld = getObjectField(e, 3);

  Kind e_kind = (Kind)readUInt8(kindFld);
  if (e_kind == Kind::Add) {
    // const Expr *e1 = ((AddExpr *)e)->left;
    Object *e1 = readPointer(leftFld);
    Field *e1_kindFld = getObjectField(e1, 0);
    Kind e1_kind = (Kind)readUInt8(e1_kindFld);

    // const Expr *e2 = ((AddExpr *)e)->right;
    Object *e2 = readPointer(rightFld);
    Field *e2_kindFld = getObjectField(e2, 0);
    Kind e2_kind = (Kind)readUInt8(e2_kindFld);

    if (e1_kind == Kind::Val && e2_kind == Kind::Val) {
      // ((ValExpr *)e1)->value
      auto e1ValFld = getObjectField(e1, 1);
      auto e1_value = readInt64(e1ValFld);

      // ((ValExpr *)e2)->value
      auto e2ValFld = getObjectField(e2, 1);
      auto e2_value = readInt64(e2ValFld);

      // return new ValExpr(((ValExpr *)e1)->value
      //                    + ((ValExpr *)e2)->value);
      return new_ValExpr(e1_value + e2_value);
    } else if (e1_kind == Val && e2_kind == Add) {
      auto e2_rightFld = getObjectField(e2, 3);
      auto e2_right = readPointer(e2_rightFld);
      auto e2_right_kindFld = getObjectField(e2_right, 0);
      auto e2_right_kind =
          (Kind)readUInt8(e2_right_kindFld);
      if (e2_right_kind == Kind::Val) {
        auto b = e2;
        auto v = e2_right;

        //     new ValExpr(((ValExpr *)e1)->value +
        //     v->value)
        auto e1_valueFld = getObjectField(e1, 1);
        auto e1_value = readInt64(e1_valueFld);
        auto v_valueFld = getObjectField(v, 1);
        auto v_value = readInt64(v_valueFld);

        auto b_leftFld = getObjectField(b, 2);
        auto b_left = readPointer(b_leftFld);

        // return new AddExpr(
        //     new ValExpr(((ValExpr *)e1)->value +
        //     v->value), b->left);
        return new_AddExpr(new_ValExpr(e1_value + v_value),
                           b_left);
      }

      auto e2_leftFld = getObjectField(e2, 3);
      auto e2_left = readPointer(e2_leftFld);
      auto e2_left_kindFld = getObjectField(e2_left, 0);
      auto e2_left_kind = (Kind)readUInt8(e2_left_kindFld);

      if (e2_left_kind == Kind::Val) {
        // AddExpr *b = (AddExpr *)e2;
        auto b = e2;
        // ValExpr *v = (ValExpr *)(b->left);
        auto b_leftFld = getObjectField(b, 2);
        auto b_left = readPointer(b_leftFld);
        auto v = b_left;

        //     new ValExpr(((ValExpr *)e1)->value +
        //     v->value),
        auto e1ValFld = getObjectField(e1, 1);
        auto e1_value = readInt64(e1ValFld);
        auto v_valueFld = getObjectField(v, 1);
        auto v_value = readInt64(v_valueFld);

        auto newValExpr = new_ValExpr(e1_value + v_value);

        // return new AddExpr(
        //     new ValExpr(
        //       ((ValExpr *)e1)->value +
        //       v->value),
        //     b->right);
        auto b_rightFld = getObjectField(b, 3);
        auto b_right = readPointer(b_rightFld);

        return new_AddExpr(newValExpr, b_right);
      }
    }

    // return new AddExpr(e1, e2);
    return new_AddExpr(e1, e2);
  } else if (e_kind == Kind::Mul) {
    // const Expr *e1 = ((MulExpr *)e)->left;
    auto e_leftFld = getObjectField(e, 2);
    auto e_left = readPointer(e_leftFld);
    auto e1 = e_left;

    // const Expr *e2 = ((MulExpr *)e)->right;
    auto e_rightFld = getObjectField(e, 3);
    auto e_right = readPointer(e_rightFld);
    auto e2 = e_right;

    // e1->kind
    auto e1_kindFld = getObjectField(e1, 0);
    auto e1_kind = (Kind)readUInt8(e1_kindFld);

    // e2->kind
    auto e2_kindFld = getObjectField(e2, 0);
    auto e2_kind = (Kind)readUInt8(e2_kindFld);

    if (e1_kind == Kind::Val && e2_kind == Kind::Val) {
      // return new ValExpr(((ValExpr *)e1)->value
      //                    * ((ValExpr *)e2)->value);
      auto e1_valueFld = getObjectField(e1, 1);
      auto e1_value = readInt64(e1_valueFld);
      auto e2_valueFld = getObjectField(e2, 1);
      auto e2_value = readInt64(e2_valueFld);

      return new_ValExpr(e1_value * e2_value);
    } else if (e1_kind == Kind::Val
               && e2_kind == Kind::Mul) {
      auto e2_rightFld = getObjectField(e2, 3);
      auto e2_right = readPointer(e2_rightFld);
      auto e2_right_kindFld = getObjectField(e2_right, 0);
      auto e2_right_kind =
          (Kind)readUInt8(e2_right_kindFld);

      if (e2_right_kind == Kind::Val) {
        auto b = e2;
        auto v = e2_right;

        // return new MulExpr(
        //     new ValExpr(((ValExpr *)e1)->value *
        //     v->value), b->left);
        auto e1_valueFld = getObjectField(e1, 1);
        auto e1_value = readInt64(e1_valueFld);
        auto v_valueFld = getObjectField(v, 1);
        auto v_value = readInt64(v_valueFld);

        auto newValExpr = new_ValExpr(e1_value * v_value);

        auto b_leftFld = getObjectField(e2, 2);
        auto b_left = readPointer(b_leftFld);

        return new_MulExpr(newValExpr, b_left);
      }

      auto e2_leftFld = getObjectField(e2, 2);
      auto e2_left = readPointer(e2_leftFld);
      auto e2_left_kindFld = getObjectField(e2_left, 0);
      auto e2_left_kind = (Kind)readUInt8(e2_left_kindFld);

      if (e2_left_kind == Kind::Val) {
        auto b = e2;
        auto v = e2_left;

        // return new MulExpr(
        //     new ValExpr(((ValExpr *)e1)->value *
        //     v->value), b->right);
        auto e1_valueFld = getObjectField(e1, 1);
        auto e1_value = readInt64(e1_valueFld);
        auto v_valueFld = getObjectField(v, 1);
        auto v_value = readInt64(v_valueFld);
        auto b_rightFld = getObjectField(e2, 2);
        auto b_right = readPointer(b_rightFld);

        return new_MulExpr(new_ValExpr(e1_value * v_value),
                           b_right);
      }

      // return new MulExpr(e1, e2);
      return new_MulExpr(e1, e2);
    }
  }

  return e;
}

static int64_t eval(Object *e) {
  assertType(Expr, e);

  auto e_kindFld = getObjectField(e, 0);
  auto e_kind = (Kind)readUInt8(e_kindFld);

  if (e_kind == Kind::Var) {
    return 0;
  } else if (e_kind == Val) {
    auto e_valueFld = getObjectField(e, 1);
    auto e_value = readInt64(e_valueFld);

    return e_value;
  } else if (e_kind == Add) {
    auto e_leftFld = getObjectField(e, 2);
    auto e_rightFld = getObjectField(e, 3);
    auto e_left = readPointer(e_leftFld);
    auto e_right = readPointer(e_rightFld);

    return eval(e_left) + eval(e_right);
  } else if (e_kind == Mul) {
    auto e_leftFld = getObjectField(e, 2);
    auto e_rightFld = getObjectField(e, 3);
    auto e_left = readPointer(e_leftFld);
    auto e_right = readPointer(e_rightFld);

    return eval(e_left) * eval(e_right);
  } else {
    return 0;
  }
}

int main(int argc, char **argv) {
  Object *e = mk_expr(20, 1);
  int64_t v1 = eval(e);
  int64_t v2 = eval(const_folding(reassoc(e)));
  std::cout << v1 << ", " << v2 << "\n";
  return 0;
}
