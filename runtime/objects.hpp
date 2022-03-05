/*
 * Object representation recognizable by LLVM IR
 * Author(s): Tommy McMichen
 * Created: Mar 4, 2022
 */

namespace objectir {

struct Field {
  uint64_t bitwidth;
  bool isPointer;

  std::string toString();
};

struct NullField {
  NullField();
  
  std::string toString();
};

struct IntegerField : public Field {
  uint64_t value;
  uint64_t bitwidth;

  IntegerField(uint64_t value, uint64_t bitwidth);

  std::string toString();
};

template <class T : Object> struct PointerField : public PointerField {
  T *obj;

  PointerField(T *obj);

  std::string toString();
}

struct Object {
  std::vector<Field *> fields;

  Object(std::vector<Field *> fields);

  Field *readField(uint64_t fieldNo);
  Field *writeField(uint64_t fieldNo);

  virtual std::string toString() = 0;
};

struct Array : public Object {
  std::vector<Field *> elements;
  uint64_t length;

  Array(uint64_t length);

  std::string toString();
};

struct LinkedList : public Object {
  PointerField *next;

  LinkedList(PointerField *next);

  void setNext(PointerField *prev);

  std::string toString();
};

struct DoublyLinkedList : public LinkedList {
  PointerField *prev;

  DoublyLinkedList(PointerField *next, PointerField *prev);

  void setPrev(PointerField *prev);

  std::string toString();
};

struct Tree : public Object {
  std::vector<Tree *> children;

  Tree(std::vector<Field *> children);

  void addChild(Tree *child);
  void getChild(uint64_t childNo);

  std::string toString();
};

struct Graph : public Object {
  std::unordered_set<Graph *> outgoing;

  Graph(std::vector<Graph *> outgoing);

  void addEdge(Graph *to);

  std::string toString();
};

} // namespace objectir
