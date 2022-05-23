// Using standard STL to test the red-black tree in C++
// In glibc++ this uses
// <https://github.com/gcc-mirror/gcc/tree/master/libstdc++-v3/src/c++98/tree.cc>
// With the LLVM libc++ this uses
// <https://github.com/llvm/llvm-project/blob/main/libcxx/include/__tree>
// In glibc this uses eventually:
// <https://sourceware.org/git/?p=glibc.git;a=blob;f=misc/tsearch.c>
// (Highly optimized in-place red-black tree using the low
// pointer bit to encode color information.)

#include <iostream>

/*
 * This implementation is based on the Java implementation
 * since the C++ version uses strd::map's internal rb-tree
 * which is too hacky and we want a more grounded
 * implementation.
 */

enum Color {
  Red,  // 0
  Black // 1
}

Type *Tree = nameObjectType(
    "Tree",
    5,
    getBooleanType(),                     // Color
    getPointerType(getNamedType("Tree")), // left
    getInt32Type(),                       // key
    getBooleanType(1, false),             // val
    getPointerType(getNamedType("Tree"))  // right
);

Object *new_Tree(Color c,
                 Object *l,
                 int32_t k,
                 boolean v,
                 Object *r) {
  setReturnType(Tree);
  assertType(Tree, l);
  assertType(Tree, r);

  auto newTree = buildObject(Tree);
  writeBoolean(getObjectField(newTree, 0), (boolean)c);
  writePointer(getObjectField(newTree, 1), l);
  writeInt32(getObjectField(newTree, 2), k);
  writeBoolean(getObjectField(newTree, 3), v);
  writePointer(getObjectField(newTree, 4), r);

  return newTree;
}

boolean isRed(Object *t) {
  assertType(Tree, t);

  if (t == nullptr) {
    return false;
  }

  auto c = (Color)readBoolean(getObjectField(t, 0));

  return c == Color::Red;
}

Object *balanceRight(int kv,
                     boolean vv,
                     Object *t,
                     Object *n) {
  setReturnType(Tree);
  assertType(Tree, t);
  assertType(Tree, n);

  if (n == nullptr) {
    return nullptr;
  }

  auto n_leftFld = getObjectField(n, 1);
  auto n_left = readPointer(n_leftFld);
  if (n_left != nullptr) {
    auto l = n_left;

    auto l_colorFld = getObjectField(l, 0);
    auto l_color = (Color)readBoolean(l);
    if (l_color == Color::Red) {
      auto l_left = readPointer(getObjectField(l, 1));
      auto l_key = readInt32(getObjectField(l, 2));
      auto l_val = readBoolean(getObjectField(l, 3));
      auto l_right = readPointer(getObjectField(l, 4));
      auto newLeft = new_Tree(Color::Black,
                              l_left,
                              l_key,
                              l_val,
                              l_right);

      auto n_right = readPointer(getObjectField(n, 4));
      auto newRight =
          new_Tree(Color::Black, n_right, kv, vv, t);

      auto n_key = readInt32(getObjectField(n, 2));
      auto n_val = readBoolean(getObjectField(n, 3));
      return new_Tree(Color::Red,
                      newLeft,
                      n_key,
                      n_val,
                      newRight);
    }

    // TODO
  }
}
