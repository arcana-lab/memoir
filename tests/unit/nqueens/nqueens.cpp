// NQueens solution in C++
// Note: does not free memory as that is difficult to do
// since many subsolutions are shared
#include <iostream>

#include "object_ir.h"

using namespace objectir;

/*
template <typename T>
class list {
public:
  T head;
  list<T>* tail;
  list(T hd, list<T>* tl) {
    head = hd;
    tail = tl;
  }
  ~list() {
    delete head;
    delete tail;
  }
};
*/

Type *ListOfInts = nameObjectType( // List<int>
    "IntList",
    2,
    getUInt32Type(),                        // head
    getPointerType(getNamedType("IntList")) // tail
);

Type *ListOfLists = nameObjectType( // List<List<int> *>
    "ListList",
    2,
    getPointerType(getNamedType("IntList")), // head
    getPointerType(getNamedType("ListList")) // tail
);

/*
template <typename T>
list<T> *Cons(T hd, list<T> *tl) {
  return new list<T>(hd, tl);
}
*/

Object *Cons_int(uint32_t hd, Object *tl) {
  setReturnType(ListOfInts);
  assertType(ListOfInts, tl);

  Object *newList = buildObject(ListOfInts);
  Field *headFld = getObjectField(newList, 0);
  Field *tailFld = getObjectField(newList, 1);

  writeUInt32(headFld, hd);
  writePointer(tailFld, tl);

  return newList;
}

Object *Cons_list(Object *hd, Object *tl) {
  setReturnType(ListOfLists);
  assertType(ListOfInts, hd);
  assertType(ListOfLists, tl);

  Object *newList = buildObject(ListOfLists);
  Field *headFld = getObjectField(newList, 0);
  Field *tailFld = getObjectField(newList, 1);

  writePointer(headFld, hd);
  writePointer(tailFld, tl);

  return newList;
}

/*
template <typename T>
int len(list<T> *xs) {
  int n = 0;
  while (xs != NULL) {
    n++;
    xs = xs->tail;
  }
  return n;
}
*/

uint32_t len_int(Object *xs) {
  assertType(ListOfInts, xs);

  uint32_t n = 0;
  while (xs != NULL) {
    n++;

    // xs = xs->tail;
    Field *tailFld = getObjectField(xs, 1);
    xs = readPointer(tailFld);
  }
  return n;
}

uint32_t len_list(Object *xs) {
  assertType(ListOfLists, xs);

  uint32_t n = 0;
  while (xs != NULL) {
    n++;
    // xs = xs->tail;
    Field *tailFld = getObjectField(xs, 1);
    xs = readPointer(tailFld);
  }
  return n;
}

/*
bool safe(uint32_t queen, list<uint32_t> *xs) {
  list<uint32_t> *cur = xs;
  uint32_t diag = 1;
  while (cur != NULL) {
    uint32_t q = cur->head;
    if (queen == q || queen == (q + diag)
        || queen == (q - diag)) {
      return false;
    }
    diag++;
    cur = cur->tail;
  }
  return true;
}
*/
bool safe(uint32_t queen, Object *xs) {
  assertType(ListOfInts, xs);

  Object *cur = xs;
  uint32_t diag = 1;
  while (cur != NULL) {
    // uint32_t q = cur->head;
    Field *headFld = getObjectField(cur, 0);
    uint32_t q = readUInt32(headFld);

    if (queen == q || queen == (q + diag)
        || queen == (q - diag)) {
      return false;
    }

    diag++;
    // cur = cur->tail;
    Field *tailFld = getObjectField(cur, 1);
    cur = readPointer(tailFld);
  }

  return true;
}

Object *append_safe(uint32_t k,
                    Object *soln,
                    Object *solns) {
  setReturnType(ListOfLists);
  assertType(ListOfInts, soln);
  assertType(ListOfLists, solns);

  Object *acc = solns;
  uint32_t n = k;
  while (n > 0) {
    if (safe(n, soln)) {
      acc = Cons_list(Cons_int(n, soln), acc);
    }
    n--;
  }
  return acc;
}

/*
list<list<uint32_t> *> *extend(
    uint32_t n,
    list<list<uint32_t> *> *solns) {
  list<list<uint32_t> *> *acc = NULL;
  list<list<uint32_t> *> *cur = solns;
  while (cur != NULL) {
    list<uint32_t> *soln = cur->head;
    acc = append_safe(n, soln, acc);
    cur = cur->tail;
  }
  return acc;
}
*/

Object *extend(uint32_t n, Object *solns) {
  setReturnType(ListOfLists);
  assertType(ListOfLists, solns);

  Object *acc = NULL;
  Object *cur = solns;
  while (cur != NULL) {
    //Object *soln = cur->head;
    Field *headFld = getObjectField(cur, 0);
    Object *soln = readPointer(headFld);
    
    acc = append_safe(n, soln, acc);
    
    // cur = cur->tail;
    Field *tailFld = getObjectField(cur, 1);
    cur = readPointer(tailFld);
  }
  return acc;
}
/*
list<list<uint32_t> *> *find_solutions(uint32_t n) {
  uint32_t k = 0;
  list<list<uint32_t> *> *acc =
      Cons<list<uint32_t> *>(NULL, NULL);
  while (k < n) {
    acc = extend(n, acc);
    k++;
  }
  return acc;
}
*/
Object *find_solutions(uint32_t n) {
  setReturnType(ListOfLists);
  
  uint32_t k = 0;
  //list<list<uint32_t> *> *acc =
  //    Cons<list<uint32_t> *>(NULL, NULL);
  Object *acc = buildObject(ListOfLists);
  while (k < n) {
    acc = extend(n, acc);
    k++;
  }
  return acc;
}


uint32_t nqueens(uint32_t n) {
  return len_list(find_solutions(n));
}

int main(int argc, char **argv) {
  int n = 13;
  if (argc == 2) {
    n = atoi(argv[1]);
  }
  std::cout << nqueens(n) << "\n";
  return 0;
}
