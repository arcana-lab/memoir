#include <iostream>
#include <math.h>
#include <string.h>

#include "memoir.h"

using namespace memoir;

enum Kind {
  Val, // 0
  Var, // 1
  Add, // 2
  Mul, // 3
};

Type *Expr = defineStructType("Expr",      // merged subclasses
                              4,           // # fields
                              UInt8Type(), // kind
                              Int64Type(), // name/value
                              ReferenceType(StructType("Expr")), // left
                              ReferenceType(StructType("Expr"))  // right
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

  auto valExpr = allocateStruct(Expr);
  auto valExpr_kindFld = getStructField(valExpr, 0);
  auto valExpr_valueFld = getStructField(valExpr, 1);
  writeUInt8(valExpr_kindFld, (uint8_t)Kind::Val);
  writeInt64(valExpr_valueFld, i);

  return valExpr;
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

  auto varExpr = allocateStruct(Expr);
  auto varExpr_kindFld = getStructField(varExpr, 0);
  auto varExpr_nameFld = getStructField(varExpr, 1);
  writeUInt8(varExpr_kindFld, (uint8_t)Kind::Var);
  writeInt64(varExpr_nameFld, n);

  return varExpr;
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

  auto addExpr = allocateStruct(Expr);
  auto addExpr_kindFld = getStructField(addExpr, 0);
  auto addExpr_leftFld = getStructField(addExpr, 2);
  auto addExpr_rightFld = getStructField(addExpr, 3);
  writeUInt8(addExpr_kindFld, (uint8_t)Kind::Add);
  writeReference(addExpr_leftFld, left);
  writeReference(addExpr_rightFld, right);

  return addExpr;
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

  auto mulExpr = allocateStruct(Expr);
  auto mulExpr_kindFld = getStructField(mulExpr, 0);
  auto mulExpr_leftFld = getStructField(mulExpr, 2);
  auto mulExpr_rightFld = getStructField(mulExpr, 3);
  writeUInt8(mulExpr_kindFld, (uint8_t)Kind::Mul);
  writeReference(mulExpr_leftFld, left);
  writeReference(mulExpr_rightFld, right);

  return mulExpr;
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

Object *mk_expr(int64_t n, int64_t v) {
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

Object *append_add(Object *e1, Object *e2) {
  setReturnType(Expr);
  assertType(Expr, e1);
  assertType(Expr, e2);

  Field *e1_kindFld = getStructField(e1, 0);
  Kind e1_kind = (Kind)readUInt8(e1_kindFld);

  if (e1_kind == Kind::Add) {
    // const AddExpr *x = (AddExpr *)e1;
    auto x = e1;

    auto x_leftField = getStructField(x, 2);
    auto x_rightField = getStructField(x, 3);
    auto x_left = readReference(x_leftField);
    auto x_right = readReference(x_rightField);

    // return new AddExpr(x->left, append_add(x->right,
    // e2));
    return new_AddExpr(x_left, append_add(x_right, e2));
  } else {
    // return new AddExpr(e1, e2);
    return new_AddExpr(e1, e2);
  }
}

Object *append_mul(Object *e1, Object *e2) {
  setReturnType(Expr);
  assertType(Expr, e1);
  assertType(Expr, e2);

  auto kindFld = getStructField(e1, 0);
  auto kind = (Kind)readUInt8(kindFld);

  if (kind == Kind::Mul) {
    // const MulExpr *x = (MulExpr *)e1;
    auto x = e1;
    auto x_leftField = getStructField(x, 2);
    auto x_rightField = getStructField(x, 3);
    auto x_left = readReference(x_leftField);
    auto x_right = readReference(x_rightField);

    // return new MulExpr(x->left, append_mul(x->right,
    // e2));
    return new_MulExpr(x_left, append_mul(x_right, e2));
  } else {
    // return new MulExpr(e1, e2);
    return new_MulExpr(e1, e2);
  }
}

Object *reassoc(Object *e) {
  setReturnType(Expr);
  assertType(Expr, e);

  auto e_kindFld = getStructField(e, 0);
  auto e_kind = (Kind)readUInt8(e_kindFld);

  if (e_kind == Kind::Add) {
    // const AddExpr *x = (AddExpr *)e;
    auto x = e;

    // return append_add(reassoc(x->left),
    // reassoc(x->right));
    auto x_leftFld = getStructField(x, 2);
    auto x_rightFld = getStructField(x, 3);
    auto x_left = readReference(x_leftFld);
    auto x_right = readReference(x_rightFld);

    return append_add(reassoc(x_left), reassoc(x_right));

  } else if (e_kind == Kind::Mul) {
    // const MulExpr *x = (MulExpr *)e;
    auto x = e;

    // return append_mul(reassoc(x->left),
    // reassoc(x->right));
    auto x_leftFld = getStructField(x, 2);
    auto x_rightFld = getStructField(x, 3);
    auto x_left = readReference(x_leftFld);
    auto x_right = readReference(x_rightFld);

    return append_mul(reassoc(x_left), reassoc(x_right));

  } else {
    return e;
  }
}

Object *const_folding(Object *e) {
  setReturnType(Expr);
  assertType(Expr, e);

  auto kindFld = getStructField(e, 0);
  auto valFld = getStructField(e, 1);
  auto leftFld = getStructField(e, 2);
  auto rightFld = getStructField(e, 3);

  auto e_kind = (Kind)readUInt8(kindFld);
  if (e_kind == Kind::Add) {
    // const Expr *e1 = ((AddExpr *)e)->left;
    auto e1 = readReference(leftFld);
    auto e1_kindFld = getStructField(e1, 0);
    auto e1_kind = (Kind)readUInt8(e1_kindFld);

    // const Expr *e2 = ((AddExpr *)e)->right;
    auto e2 = readReference(rightFld);
    auto e2_kindFld = getStructField(e2, 0);
    auto e2_kind = (Kind)readUInt8(e2_kindFld);

    if (e1_kind == Kind::Val && e2_kind == Kind::Val) {
      // ((ValExpr *)e1)->value
      auto e1ValFld = getStructField(e1, 1);
      auto e1_value = readInt64(e1ValFld);

      // ((ValExpr *)e2)->value
      auto e2ValFld = getStructField(e2, 1);
      auto e2_value = readInt64(e2ValFld);

      // return new ValExpr(((ValExpr *)e1)->value
      //                    + ((ValExpr *)e2)->value);
      return new_ValExpr(e1_value + e2_value);
    } else if (e1_kind == Val && e2_kind == Add) {
      auto e2_rightFld = getStructField(e2, 3);
      auto e2_right = readReference(e2_rightFld);
      auto e2_right_kindFld = getStructField(e2_right, 0);
      auto e2_right_kind = (Kind)readUInt8(e2_right_kindFld);
      if (e2_right_kind == Kind::Val) {
        auto b = e2;
        auto v = e2_right;

        //     new ValExpr(((ValExpr *)e1)->value +
        //     v->value)
        auto e1_valueFld = getStructField(e1, 1);
        auto e1_value = readInt64(e1_valueFld);
        auto v_valueFld = getStructField(v, 1);
        auto v_value = readInt64(v_valueFld);

        auto b_leftFld = getStructField(b, 2);
        auto b_left = readReference(b_leftFld);

        // return new AddExpr(
        //     new ValExpr(((ValExpr *)e1)->value +
        //     v->value), b->left);
        return new_AddExpr(new_ValExpr(e1_value + v_value), b_left);
      }

      auto e2_leftFld = getStructField(e2, 2);
      auto e2_left = readReference(e2_leftFld);
      auto e2_left_kindFld = getStructField(e2_left, 0);
      auto e2_left_kind = (Kind)readUInt8(e2_left_kindFld);

      if (e2_left_kind == Kind::Val) {
        // AddExpr *b = (AddExpr *)e2;
        auto b = e2;

        // ValExpr *v = (ValExpr *)(b->left);
        auto b_leftFld = getStructField(b, 2);
        auto b_left = readReference(b_leftFld);
        auto v = b_left;

        //     new ValExpr(((ValExpr *)e1)->value +
        //     v->value),
        auto e1_valueFld = getStructField(e1, 1);
        auto e1_value = readInt64(e1_valueFld);
        auto v_valueFld = getStructField(v, 1);
        auto v_value = readInt64(v_valueFld);

        // return new AddExpr(
        //     new ValExpr(
        //       ((ValExpr *)e1)->value +
        //       v->value),
        //     b->right);
        auto b_rightFld = getStructField(b, 3);
        auto b_right = readReference(b_rightFld);

        return new_AddExpr(new_ValExpr(e1_value + v_value), b_right);
      }
    }

    // return new AddExpr(e1, e2);
    return new_AddExpr(e1, e2);

  } else if (e_kind == Kind::Mul) {
    // const Expr *e1 = ((MulExpr *)e)->left;
    auto e_leftFld = getStructField(e, 2);
    auto e_left = readReference(e_leftFld);
    auto e1 = e_left;

    // const Expr *e2 = ((MulExpr *)e)->right;
    auto e_rightFld = getStructField(e, 3);
    auto e_right = readReference(e_rightFld);
    auto e2 = e_right;

    // e1->kind
    auto e1_kindFld = getStructField(e1, 0);
    auto e1_kind = (Kind)readUInt8(e1_kindFld);

    // e2->kind
    auto e2_kindFld = getStructField(e2, 0);
    auto e2_kind = (Kind)readUInt8(e2_kindFld);

    if (e1_kind == Kind::Val && e2_kind == Kind::Val) {
      // return new ValExpr(((ValExpr *)e1)->value
      //                    * ((ValExpr *)e2)->value);
      auto e1_valueFld = getStructField(e1, 1);
      auto e1_value = readInt64(e1_valueFld);
      auto e2_valueFld = getStructField(e2, 1);
      auto e2_value = readInt64(e2_valueFld);

      return new_ValExpr(e1_value * e2_value);
    } else if (e1_kind == Kind::Val && e2_kind == Kind::Mul) {
      auto e2_rightFld = getStructField(e2, 3);
      auto e2_right = readReference(e2_rightFld);
      auto e2_right_kindFld = getStructField(e2_right, 0);
      auto e2_right_kind = (Kind)readUInt8(e2_right_kindFld);

      if (e2_right_kind == Kind::Val) {
        auto b = e2;
        auto v = e2_right;

        // return new MulExpr(
        //     new ValExpr(((ValExpr *)e1)->value *
        //     v->value), b->left);
        auto e1_valueFld = getStructField(e1, 1);
        auto e1_value = readInt64(e1_valueFld);
        auto v_valueFld = getStructField(v, 1);
        auto v_value = readInt64(v_valueFld);

        auto b_leftFld = getStructField(e2, 2);
        auto b_left = readReference(b_leftFld);

        return new_MulExpr(new_ValExpr(e1_value * v_value), b_left);
      }

      auto e2_leftFld = getStructField(e2, 2);
      auto e2_left = readReference(e2_leftFld);
      auto e2_left_kindFld = getStructField(e2_left, 0);
      auto e2_left_kind = (Kind)readUInt8(e2_left_kindFld);

      if (e2_left_kind == Kind::Val) {
        auto b = e2;
        auto v = e2_left;

        // return new MulExpr(
        //     new ValExpr(((ValExpr *)e1)->value *
        //     v->value), b->right);
        auto e1_valueFld = getStructField(e1, 1);
        auto e1_value = readInt64(e1_valueFld);
        auto v_valueFld = getStructField(v, 1);
        auto v_value = readInt64(v_valueFld);
        auto b_rightFld = getStructField(e2, 3);
        auto b_right = readReference(b_rightFld);

        return new_MulExpr(new_ValExpr(e1_value * v_value), b_right);
      }
    }

    // return new MulExpr(e1, e2);
    return new_MulExpr(e1, e2);
  }

  return e;
}

int64_t eval(Object *e) {
  assertType(Expr, e);

  auto e_kindFld = getStructField(e, 0);
  auto e_kind = (Kind)readUInt8(e_kindFld);

  if (e_kind == Kind::Var) {
    return 0;
  } else if (e_kind == Val) {
    auto e_valueFld = getStructField(e, 1);
    auto e_value = readInt64(e_valueFld);

    return e_value;
  } else if (e_kind == Add) {
    auto e_leftFld = getStructField(e, 2);
    auto e_rightFld = getStructField(e, 3);
    auto e_left = readReference(e_leftFld);
    auto e_right = readReference(e_rightFld);

    return eval(e_left) + eval(e_right);
  } else if (e_kind == Mul) {
    auto e_leftFld = getStructField(e, 2);
    auto e_rightFld = getStructField(e, 3);
    auto e_left = readReference(e_leftFld);
    auto e_right = readReference(e_rightFld);

    return eval(e_left) * eval(e_right);
  } else {
    return 0;
  }
}

int main(int argc, char **argv) {
  Object *e = mk_expr(15, 1);
  int64_t v1 = eval(e);
  int64_t v2 = eval(const_folding(reassoc(e)));
  std::cout << v1 << ", " << v2 << "\n";
  return 0;
}
