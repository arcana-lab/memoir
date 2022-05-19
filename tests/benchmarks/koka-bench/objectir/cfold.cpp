#include <iostream>
#include <math.h>
#include <string.h>

enum Kind {
  Val, // 0
  Var, // 1
  Add, // 2
  Mul, // 3
};

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

/*
class VarExpr : public Expr {
public:
  long name;
  VarExpr(long n) : Expr(Var) {
    this->name = n;
  }
};
*/

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
Type *Expr = nameObjectType(
    "Expr", // merged subclasses
    4,
    getUInt8Type(),                       // kind
    getInt64Type(),                       // name/value
    getPointerType(getNamedType("Expr")), // left
    getPointerType(getNamedType("Expr"))  // right
);

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
      Object *varExprObj = buildObject(Expr);
      Field *kindFld = getObjectField(varExprObj, 0);
      Field *nameFld = getObjectField(varExprObj, 1);
      writeUInt8((uint8_t)Kind::Var);
      writeInt64(1);
      return varExprObj;
    } else {
      Object *valExprObj = buildObject(Expr);
      Field *kindFld = getObjectField(valExprObj, 0);
      Field *valueFld = getObjectField(valExprObj, 1);
      writeUInt8((uint8_t)Kind::Val);
      writeInt64(v);
      return valExprObj;
    }
  } else {
    Object *addExprObj = buildObject(Expr);
    Field *kindFld = getObjectField(addExprObj, 0);
    Field *nameFld = getObjectField(addExprObj, 1);
    Field *leftFld = getObjectField(addExprObj, 2);
    Field *rightFld = getObjectField(addExprObj, 3);
    writeUInt8(kindFld, (uint8_t)Kind::Add);
    writePointer(leftFld, mk_expr(n - 1, v + 1));
    writePointer(rightFld, mk_expr(n - 1, v == 0 ? 0 : v - 1));
    return addExprObj;
    // return new AddExpr(mk_expr(n - 1, v + 1),
    //    mk_expr(n - 1, v == 0 ? 0 : v - 1));
  }
}

static Object *append_add(Object *e1, Object *e2) {
  setReturnType(Expr);
  assertType(Expr, e1);
  assertType(Expr, e2);

  Field *kindFld = getObjectField(e1, 0);
  Kind kind = (Kind)readUInt8(kindFld);
  if (kind == Add) {
    //const AddExpr *x = (AddExpr *)e1;
    x = e1;
    Field *x_leftField = getObjectField(x, 2);
    Field *x_rightField = getObjectField(x, 3);
    Object *x_left = readPointer(x_leftField);
    Object *x_right = readPointer(x_rightField);

    //return new AddExpr(x->left, append_add(x->right, e2));
    Object *addExprObj = buildObject(Expr);
    Field *kindFld = getObjectField(addExprObj, 0);
    Field *leftField = getObjectField(addExprObj, 2);
    Field *rightField = getObjectField(addExprObj, 3);
    writeUInt8(kindFld, (uint8_t)Kind::Add);
    writePointer(leftField, x_left);
    writePointer(rightField, append_add(x_right, e2));
    
    return addExprObj;
  } else {
    // return new AddExpr(e1, e2);
    Object *addExprObj = buildObject(Expr);
    Field *kindFld = getObjectField(addExprObj, 0);
    Field *leftField = getObjectField(addExprObj, 2);
    Field *rightField = getObjectField(addExprObj, 3);
    writeUInt8(kindFld, (uint8_t)Kind::Add);
    writePointer(leftField, e1);
    writePointer(rightField, e2);

    return addExprObj;
  }
}

static const Expr *append_mul(const Expr *e1,
                              const Expr *e2) {
  if (e1->kind == Mul) {
    const MulExpr *x = (MulExpr *)e1;
    return new MulExpr(x->left, append_mul(x->right, e2));
  } else {
    return new MulExpr(e1, e2);
  }
}

static const Expr *reassoc(const Expr *e) {
  if (e->kind == Add) {
    const AddExpr *x = (AddExpr *)e;
    return append_add(reassoc(x->left), reassoc(x->right));
  } else if (e->kind == Mul) {
    const MulExpr *x = (MulExpr *)e;
    return append_mul(reassoc(x->left), reassoc(x->right));
  } else
    return e;
}

static const Expr *const_folding(const Expr *e) {
  if (e->kind == Add) {
    const Expr *e1 = ((AddExpr *)e)->left;
    const Expr *e2 = ((AddExpr *)e)->right;
    if (e1->kind == Val && e2->kind == Val) {
      return new ValExpr(((ValExpr *)e1)->value
                         + ((ValExpr *)e2)->value);
    } else if (e1->kind == Val && e2->kind == Add
               && ((AddExpr *)e2)->right->kind == Val) {
      AddExpr *b = (AddExpr *)e2;
      ValExpr *v = (ValExpr *)(b->right);
      return new AddExpr(
          new ValExpr(((ValExpr *)e1)->value + v->value),
          b->left);
    } else if (e1->kind == Val && e2->kind == Add
               && ((AddExpr *)e2)->left->kind == Val) {
      AddExpr *b = (AddExpr *)e2;
      ValExpr *v = (ValExpr *)(b->left);
      return new AddExpr(
          new ValExpr(((ValExpr *)e1)->value + v->value),
          b->right);
    } else {
      return new AddExpr(e1, e2);
    }
  } else if (e->kind == Mul) {
    const Expr *e1 = ((MulExpr *)e)->left;
    const Expr *e2 = ((MulExpr *)e)->right;
    if (e1->kind == Val && e2->kind == Val) {
      return new ValExpr(((ValExpr *)e1)->value
                         * ((ValExpr *)e2)->value);
    } else if (e1->kind == Val && e2->kind == Mul
               && ((MulExpr *)e2)->right->kind == Val) {
      MulExpr *b = (MulExpr *)e2;
      ValExpr *v = (ValExpr *)(b->right);
      return new MulExpr(
          new ValExpr(((ValExpr *)e1)->value * v->value),
          b->left);
    } else if (e1->kind == Val && e2->kind == Mul
               && ((MulExpr *)e2)->left->kind == Val) {
      MulExpr *b = (MulExpr *)e2;
      ValExpr *v = (ValExpr *)(b->left);
      return new MulExpr(
          new ValExpr(((ValExpr *)e1)->value * v->value),
          b->right);
    } else {
      return new MulExpr(e1, e2);
    }
  } else
    return e;
}

static int64_t eval(const Expr *e) {
  if (e->kind == Var) {
    return 0;
  } else if (e->kind == Val) {
    return ((ValExpr *)e)->value;
  } else if (e->kind == Add) {
    return eval(((AddExpr *)e)->left)
           + eval(((AddExpr *)e)->right);
  } else if (e->kind == Mul) {
    return eval(((MulExpr *)e)->left)
           * eval(((MulExpr *)e)->right);
  } else {
    return 0;
  }
}

int main(int argc, char **argv) {
  const Expr *e = mk_expr(20, 1);
  int64_t v1 = eval(e);
  int64_t v2 = eval(const_folding(reassoc(e)));
  std::cout << v1 << ", " << v2 << "\n";
  return 0;
}
