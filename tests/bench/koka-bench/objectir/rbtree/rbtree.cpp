
/*
 * This implementation is based on the Java implementation
 * since the C++ version uses std::map's internal rb-tree
 * which is too hacky and we want a more grounded
 * implementation.
 * Author(s): Tommy McMichen
 * Date: May 24, 2022
 */

#include <iostream>

#include "object_ir.h"

using namespace objectir;

enum Color {
  Red,  // 0
  Black // 1
};

Type *Tree = nameObjectType("Tree",
                            5,
                            getUInt8Type(),                       // Color
                            getPointerType(getNamedType("Tree")), // left
                            getInt32Type(),                       // key
                            getUInt8Type(),                       // val
                            getPointerType(getNamedType("Tree"))  // right
);

Object *new_Tree(Color c, Object *l, int32_t k, bool v, Object *r) {
  setReturnType(Tree);
  assertType(Tree, l);
  assertType(Tree, r);

  auto newTree = buildObject(Tree);
  writeUInt8(getObjectField(newTree, 0), c);
  writePointer(getObjectField(newTree, 1), l);
  writeInt32(getObjectField(newTree, 2), k);
  writeUInt8(getObjectField(newTree, 3), v);
  writePointer(getObjectField(newTree, 4), r);

  return newTree;
}

bool isRed(Object *t) {
  assertType(Tree, t);

  if (t == nullptr) {
    return false;
  }

  auto c = (Color)readUInt8(getObjectField(t, 0));

  return c == Color::Red;
}

Object *balanceRight(int kv, bool vv, Object *t, Object *n) {
  setReturnType(Tree);
  assertType(Tree, t);
  assertType(Tree, n);

  if (n == nullptr) {
    return nullptr;
  }

  auto n_left = readPointer(getObjectField(n, 1));
  if (n_left != nullptr) {
    auto l = n_left;

    auto l_color = (Color)readUInt8(getObjectField(l, 0));
    if (l_color == Color::Red) {
      auto l_left = readPointer(getObjectField(l, 1));
      auto l_key = readInt32(getObjectField(l, 2));
      auto l_val = readUInt8(getObjectField(l, 3));
      auto l_right = readPointer(getObjectField(l, 4));
      auto newLeft = new_Tree(Color::Black, l_left, l_key, l_val, l_right);

      auto n_right = readPointer(getObjectField(n, 4));
      auto newRight = new_Tree(Color::Black, n_right, kv, vv, t);

      auto n_key = readInt32(getObjectField(n, 2));
      auto n_val = readUInt8(getObjectField(n, 3));
      return new_Tree(Color::Red, newLeft, n_key, n_val, newRight);
    }
  }

  auto n_right = readPointer(getObjectField(n, 4));
  if (n_right != nullptr) {
    auto r = n_right;

    auto r_colorFld = getObjectField(r, 0);
    auto r_color = (Color)readUInt8(r_colorFld);
    if (r_color == Color::Red) {
      auto n_left = readPointer(getObjectField(n, 1));
      auto n_key = readInt32(getObjectField(n, 2));
      auto n_val = readUInt8(getObjectField(n, 3));
      auto r_left = readPointer(getObjectField(r, 1));
      auto newLeft = new_Tree(Color::Black, n_left, n_key, n_val, r_left);

      auto r_right = readPointer(getObjectField(r, 4));
      auto newRight = new_Tree(Color::Black, r_right, kv, vv, t);

      auto r_key = readInt32(getObjectField(r, 2));
      auto r_val = readUInt8(getObjectField(r, 3));
      return new_Tree(Color::Red, newLeft, r_key, r_val, newRight);
    }
  }

  auto n_key = readInt32(getObjectField(n, 2));
  auto n_val = readUInt8(getObjectField(n, 3));
  return new_Tree(Color::Black,
                  new_Tree(Color::Red, n_left, n_key, n_val, n_right),
                  kv,
                  vv,
                  t);
}

Object *balanceLeft(Object *t, int kv, bool vv, Object *n) {
  setReturnType(Tree);
  assertType(Tree, t);
  assertType(Tree, n);

  if (n == nullptr) {
    return nullptr;
  }

  auto n_left = readPointer(getObjectField(n, 1));
  if (n_left != nullptr) {
    auto l = n_left;

    auto l_color = (Color)readUInt8(getObjectField(l, 0));
    if (l_color == Color::Red) {
      auto l_left = readPointer(getObjectField(l, 1));
      auto newLeft = new_Tree(Color::Black, t, kv, vv, l_left);

      auto l_right = readPointer(getObjectField(l, 4));
      auto n_key = readInt32(getObjectField(n, 2));
      auto n_val = readUInt8(getObjectField(n, 3));
      auto n_right = readPointer(getObjectField(n, 4));
      auto newRight = new_Tree(Color::Black, l_right, n_key, n_val, n_right);

      auto l_key = readInt32(getObjectField(l, 2));
      auto l_val = readUInt8(getObjectField(l, 3));
      return new_Tree(Color::Red, newLeft, l_key, l_val, newRight);
    }
  }

  auto n_right = readPointer(getObjectField(n, 4));
  if (n_right != nullptr) {
    auto r = n_right;

    auto r_colorFld = getObjectField(r, 0);
    auto r_color = (Color)readUInt8(r_colorFld);
    if (r_color == Color::Red) {
      auto n_left = readPointer(getObjectField(n, 1));
      auto newLeft = new_Tree(Color::Black, t, kv, vv, n_left);

      auto r_left = readPointer(getObjectField(r, 1));
      auto r_key = readInt32(getObjectField(r, 2));
      auto r_val = readUInt8(getObjectField(r, 3));
      auto r_right = readPointer(getObjectField(r, 4));
      auto newRight = new_Tree(Color::Black, r_left, r_key, r_val, n_right);

      auto n_key = readInt32(getObjectField(n, 2));
      auto n_val = readUInt8(getObjectField(n, 3));
      return new_Tree(Color::Red, newLeft, n_key, n_val, newRight);
    }
  }

  auto n_key = readInt32(getObjectField(n, 2));
  auto n_val = readUInt8(getObjectField(n, 3));
  return new_Tree(Color::Black,
                  t,
                  kv,
                  vv,
                  new_Tree(Color::Red, n_left, n_key, n_val, n_right));
}

Object *ins(Object *t, int32_t kx, bool vx) {
  setReturnType(Tree);
  assertType(Tree, t);
  if (t == nullptr) {
    return new_Tree(Color::Red, nullptr, kx, vx, nullptr);
  }

  auto t_color = (Color)readUInt8(getObjectField(t, 0));
  if (t_color == Color::Red) {
    auto t_key = readInt32(getObjectField(t, 2));
    if (kx < t_key) {
      auto t_left = readPointer(getObjectField(t, 1));
      auto t_key = readInt32(getObjectField(t, 2));
      auto t_val = readUInt8(getObjectField(t, 3));
      auto t_right = readPointer(getObjectField(t, 4));
      return new_Tree(Color::Red, ins(t_left, kx, vx), t_key, t_val, t_right);
    } else if (t_key == kx) {
      auto t_left = readPointer(getObjectField(t, 1));
      auto t_right = readPointer(getObjectField(t, 4));
      return new_Tree(Color::Red, t_left, kx, vx, t_right);
    } else {
      auto t_left = readPointer(getObjectField(t, 1));
      auto t_key = readInt32(getObjectField(t, 2));
      auto t_val = readUInt8(getObjectField(t, 3));
      auto t_right = readPointer(getObjectField(t, 4));
      return new_Tree(Color::Red, t_left, t_key, t_val, ins(t_right, kx, vx));
    }
  } else { // t_color == Black
    auto t_key = readInt32(getObjectField(t, 2));
    if (kx < t_key) {
      auto t_left = readPointer(getObjectField(t, 1));
      if (isRed(t_left)) {
        auto t_key = readInt32(getObjectField(t, 2));
        auto t_val = readUInt8(getObjectField(t, 3));
        auto t_right = readPointer(getObjectField(t, 4));
        return balanceRight(t_key, t_val, t_right, ins(t_left, kx, vx));
      } else {
        auto t_key = readInt32(getObjectField(t, 2));
        auto t_val = readUInt8(getObjectField(t, 3));
        auto t_right = readPointer(getObjectField(t, 4));
        return new_Tree(Color::Black,
                        ins(t_left, kx, vx),
                        t_key,
                        t_val,
                        t_right);
      }
    } else if (kx == t_key) {
      auto t_left = readPointer(getObjectField(t, 1));
      auto t_key = readInt32(getObjectField(t, 2));
      auto t_val = readUInt8(getObjectField(t, 3));
      auto t_right = readPointer(getObjectField(t, 4));
      return new_Tree(Color::Black, t_left, kx, vx, t_right);
    } else {
      auto t_right = readPointer(getObjectField(t, 4));
      if (isRed(t_right)) {
        auto t_left = readPointer(getObjectField(t, 1));
        auto t_key = readInt32(getObjectField(t, 2));
        auto t_val = readUInt8(getObjectField(t, 3));
        return balanceLeft(t_left, t_key, t_val, ins(t_right, kx, vx));
      } else {
        auto t_left = readPointer(getObjectField(t, 1));
        auto t_key = readInt32(getObjectField(t, 2));
        auto t_val = readUInt8(getObjectField(t, 3));
        return new_Tree(Color::Black,
                        t_left,
                        t_key,
                        t_val,
                        ins(t_right, kx, vx));
      }
    }
  }
}

Object *setBlack(Object *t) {
  setReturnType(Tree);
  assertType(Tree, t);

  if (t == nullptr) {
    return t;
  }

  auto t_left = readPointer(getObjectField(t, 1));
  auto t_key = readInt32(getObjectField(t, 2));
  auto t_val = readUInt8(getObjectField(t, 3));
  auto t_right = readPointer(getObjectField(t, 4));
  return new_Tree(Color::Black, t_left, t_key, t_val, t_right);
}

Object *insert(Object *t, int32_t k, bool v) {
  if (isRed(t)) {
    return setBlack(ins(t, k, v));
  } else {
    return ins(t, k, v);
  }
}

int FoldFun(int32_t k, uint8_t v, int acc) {
  return v ? acc + 1 : acc;
}

int Fold(Object *t, int acc) {
  assertType(Tree, t);

  while (t != nullptr) {
    auto t_left = readPointer(getObjectField(t, 1));
    auto t_key = readInt32(getObjectField(t, 2));
    auto t_val = readUInt8(getObjectField(t, 3));
    acc = Fold(t_left, acc);
    acc = FoldFun(t_key, t_val, acc);
    t = readPointer(getObjectField(t, 4));
  }

  return acc;
}

Object *mkMap(int n) {
  setReturnType(Tree);

  Object *t = nullptr;
  while (n > 0) {
    n--;
    t = insert(t, n, (n % 10) == 0);
  }

  return t;
}

int Test(int n) {
  Object *t = mkMap(n);
  return Fold(t, 0);
}

int main() {
  std::cout << Test(4200000);
  return 0;
}
