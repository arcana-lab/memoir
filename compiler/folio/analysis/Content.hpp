#include "memoir/ir/Types.hpp"

#include "memoir/support/InternalDatatypes.hpp"

namespace folio {

enum ContentKind {
  CONTENT_UNDERDEFINED,
  CONTENT_EMPTY,
  CONTENT_COLLECTION,
  CONTENT_VALUE,
  CONTENT_KEYS,
  CONTENT_ELEMENTS,
  CONTENT_FIELD,
  CONTENT_CONDITIONAL,
  CONTENT_TUPLE,
  CONTENT_UNION,
};

/**
 * An abstract content.
 */
struct Content {
  Content(ContentKind kind) : _kind(kind) {}

protected:
  ContentKind _kind;
};

/**
 * Represents an underdefined content, this can be thought of as the tautology:
 *   X's contents are comprised of X's contents.
 */
struct UnderdefinedContent : public Content {
  UnderdefinedContent() : Content(ContentKind::CONTENT_UNDERDEFINED) {}
};

/**
 * Represents an empty content.
 */
struct EmptyContent : public Content {
  EmptyContent() : Content(ContentKind::CONTENT_EMPTY) {}
};

/**
 * Represents an SSA collection.
 */
struct CollectionContent : public Content {
  CollectionContent(llvm::Value &collection)
    : Content(ContentKind::CONTENT_COLLECTION),
      _collection(collection) {}

protected:
  llvm::Value &_collection;
};

/**
 * Represents an SSA scalar.
 */
struct ValueContent : public Content {
  ValueContent(llvm::Value &value)
    : Content(ContentKind::CONTENT_VALUE),
      _value(value) {}

protected:
  llvm::Value &_value;
};

/**
 * Represents the keys of a given collection.
 */
struct KeysContent : public Content {
  KeysContent(llvm::Value &collection)
    : Content(ContentKind::CONTENT_KEYS),
      _collection(collection) {}

protected:
  llvm::Value &_collection;
};

/**
 * Represents the elements of a given collection.
 */
struct ElementsContent : public Content {
  ElementsContent(llvm::Value &collection)
    : Content(ContentKind::CONTENT_ELEMENTS),
      _collection(collection) {}

protected:
  llvm::Value &_collection;
};

/**
 * Represents a single field of a given collection's elements.
 */
struct FieldContent : public Content {
  FieldContent(llvm::Value &collection, unsigned field_index)
    : Content(ContentKind::CONTENT_FIELD),
      _collection(collection),
      _field_index(field_index) {}

protected:
  llvm::Value &_collection;
  unsigned _field_index;
};

/**
 * Represents a conditional applied to the given contents.
 */
struct ConditionalContent : public Content {
public:
  ConditionalContent(Content &content,
                     llvm::CmpInst::Predicate pred,
                     Content &lhs,
                     Content &rhs)
    : Content(ContentKind::CONTENT_CONDITIONAL),
      _content(content),
      _predicate(pred),
      _lhs(lhs),
      _rhs(rhs) {}

  bool same_conditional(const ConditionalContent &other) const {
    return (this->_predicate == other._predicate)
           and (&this->_lhs == &other._lhs) and (&this->_rhs == &other._rhs);
  }

protected:
  Content &_content;

  llvm::CmpInst::Predicate _predicate;
  Content &_lhs;
  Content &_rhs;
};

/**
 * Represents an organization of the given contents.
 */
struct TupleContent : public Content {
public:
  TupleContent(std::initializer_list<Content *> elements)
    : Content(ContentKind::CONTENT_TUPLE),
      _elements(std::forward<std::initializer_list<Content *>>(elements)) {}

protected:
  llvm::memoir::vector<Content *> _elements = {};
};

/**
 * Represents the union of two contents.
 */
struct UnionContent : public Content {
public:
  UnionContent(Content &lhs, Content &rhs)
    : Content(ContentKind::CONTENT_UNION),
      _lhs(lhs),
      _rhs(rhs) {}

protected:
  Content &_lhs;
  Content &_rhs;
};

} // namespace folio
