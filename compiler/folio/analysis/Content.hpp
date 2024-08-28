#include <numeric>
#include <string>

#include "memoir/ir/Types.hpp"

#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

namespace folio {

enum ContentKind {
  CONTENT_UNDERDEFINED,
  CONTENT_EMPTY,
  CONTENT_COLLECTION,
  CONTENT_STRUCT,
  CONTENT_SCALAR,
  CONTENT_KEY,
  CONTENT_KEYS,
  CONTENT_ELEMENT,
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

  template <typename C,
            typename... Args,
            std::enable_if_t<std::is_base_of_v<Content, C>, bool> = true>
  static Content &create(Args &&...args) {
    return MEMOIR_SANITIZE(new C(std::forward<Args>(args)...),
                           "Failed to allocate Content");
  }

  Content(ContentKind kind) : _kind(kind) {}

  virtual std::string to_string() const = 0;

  virtual bool operator==(Content &other) const = 0;

  bool operator!=(Content &other) const {
    return not(*this == other);
  }

  virtual Content &substitute(Content &from, Content &to) = 0;

  virtual ~Content() {}

  ContentKind kind() const {
    return _kind;
  }

protected:
  ContentKind _kind;
};

/**
 * Represents an underdefined content, this can be thought of as the tautology:
 *   X's contents are comprised of X's contents.
 */
struct UnderdefinedContent : public Content {
  UnderdefinedContent() : Content(ContentKind::CONTENT_UNDERDEFINED) {}

  std::string to_string() const override {
    return "⊥";
  }

  bool operator==(Content &other) const override {
    return llvm::memoir::isa<UnderdefinedContent>(&other);
  }

  Content &substitute(Content &from, Content &to) override {
    return *this;
  }

  static bool classof(const Content *content) {
    return content->kind() == ContentKind::CONTENT_UNDERDEFINED;
  }
};

/**
 * Represents an empty content.
 */
struct EmptyContent : public Content {
  EmptyContent() : Content(ContentKind::CONTENT_EMPTY) {}

  std::string to_string() const override {
    return "∅";
  }

  bool operator==(Content &other) const override {
    return llvm::memoir::isa<EmptyContent>(&other);
  }

  Content &substitute(Content &from, Content &to) override {
    return *this;
  }

  static bool classof(const Content *content) {
    return content->kind() == ContentKind::CONTENT_EMPTY;
  }
};

/**
 * Represents a struct.
 */
struct StructContent : public Content {
  StructContent(llvm::Value &value)
    : Content(ContentKind::CONTENT_STRUCT),
      _value(value) {}

  std::string to_string() const override {
    return "struct(" + llvm::memoir::value_name(this->_value) + ")";
  }

  bool operator==(Content &other) const override {
    auto *other_struct = llvm::memoir::dyn_cast<StructContent>(&other);
    if (not other_struct) {
      return false;
    }

    return &this->_value == &other_struct->_value;
  }

  bool operator==(llvm::Value &V) const {
    return &this->_value == &V;
  }

  Content &substitute(Content &from, Content &to) override {
    if (from == *this) {
      return to;
    }
    return *this;
  }

  static bool classof(const Content *content) {
    return content->kind() == ContentKind::CONTENT_STRUCT;
  }

  llvm::Value &value() {
    return this->_value;
  }

protected:
  llvm::Value &_value;
};

/**
 * Represents an SSA scalar.
 */
struct ScalarContent : public Content {
  ScalarContent(llvm::Value &value)
    : Content(ContentKind::CONTENT_SCALAR),
      _value(value) {}

  std::string to_string() const override {
    return "scalar(" + llvm::memoir::value_name(this->_value) + ")";
  }

  bool operator==(Content &other) const override {
    auto *other_scalar = llvm::memoir::dyn_cast<ScalarContent>(&other);
    if (not other_scalar) {
      return false;
    }

    return &this->_value == &other_scalar->_value;
  }

  bool operator==(llvm::Value &V) const {
    return &this->_value == &V;
  }

  Content &substitute(Content &from, Content &to) override {
    if (from == *this) {
      return to;
    }
    return *this;
  }

  static bool classof(const Content *content) {
    return content->kind() == ContentKind::CONTENT_SCALAR;
  }

  llvm::Value &value() {
    return this->_value;
  }

protected:
  llvm::Value &_value;
};

/**
 * Represents a key of a given collection.
 */
struct KeyContent : public Content {
  KeyContent(Content &collection)
    : Content(ContentKind::CONTENT_KEY),
      _collection(collection) {}

  std::string to_string() const override {
    return "key(" + this->_collection.to_string() + ")";
  }

  bool operator==(Content &other) const override {
    auto *other_key = llvm::memoir::dyn_cast<KeyContent>(&other);
    if (not other_key) {
      return false;
    }

    return this->_collection == other_key->_collection;
  }

  Content &substitute(Content &from, Content &to) override {
    auto &subst = this->_collection.substitute(from, to);
    if (&subst == &this->_collection) {
      return *this;
    }
    return Content::create<KeyContent>(subst);
  }

  static bool classof(const Content *content) {
    return content->kind() == ContentKind::CONTENT_KEY;
  }

  Content &collection() {
    return this->_collection;
  }

protected:
  Content &_collection;
};

/**
 * Represents the keys of a given collection.
 */
struct KeysContent : public Content {
  KeysContent(llvm::Value &collection)
    : Content(ContentKind::CONTENT_KEYS),
      _collection(collection) {}

  std::string to_string() const override {
    return "keys(" + llvm::memoir::value_name(this->_collection) + ")";
  }

  bool operator==(Content &other) const override {
    auto *other_keys = llvm::memoir::dyn_cast<KeysContent>(&other);
    if (not other_keys) {
      return false;
    }

    return &this->_collection == &other_keys->_collection;
  }

  Content &substitute(Content &from, Content &to) override {
    if (from == *this) {
      return to;
    }
    return *this;
  }

  static bool classof(const Content *content) {
    return content->kind() == ContentKind::CONTENT_KEYS;
  }

  llvm::Value &collection() {
    return this->_collection;
  }

protected:
  llvm::Value &_collection;
};

/**
 * Represents an element of a given collection.
 */
struct ElementContent : public Content {
  ElementContent(Content &collection)
    : Content(ContentKind::CONTENT_ELEMENT),
      _collection(collection) {}

  std::string to_string() const override {
    return "elem(" + this->_collection.to_string() + ")";
  }

  bool operator==(Content &other) const override {
    auto *other_element = llvm::memoir::dyn_cast<ElementContent>(&other);
    if (not other_element) {
      return false;
    }

    return this->_collection == other_element->_collection;
  }

  Content &substitute(Content &from, Content &to) override {
    auto &subst = this->_collection.substitute(from, to);
    if (&subst == &this->_collection) {
      return *this;
    }
    return Content::create<ElementContent>(subst);
  }

  static bool classof(const Content *content) {
    return content->kind() == ContentKind::CONTENT_ELEMENT;
  }

  Content &collection() {
    return this->_collection;
  }

protected:
  Content &_collection;
};

/**
 * Represents the elements of a given collection.
 */
struct ElementsContent : public Content {
  ElementsContent(llvm::Value &collection)
    : Content(ContentKind::CONTENT_ELEMENTS),
      _collection(collection) {}

  std::string to_string() const override {
    return "elems(" + llvm::memoir::value_name(this->_collection) + ")";
  }

  bool operator==(Content &other) const override {
    auto *other_elements = llvm::memoir::dyn_cast<ElementsContent>(&other);
    if (not other_elements) {
      return false;
    }

    return &this->_collection == &other_elements->_collection;
  }

  Content &substitute(Content &from, Content &to) override {
    if (from == *this) {
      return to;
    }
    return *this;
  }

  static bool classof(const Content *content) {
    return content->kind() == ContentKind::CONTENT_ELEMENTS;
  }

  llvm::Value &collection() {
    return this->_collection;
  }

protected:
  llvm::Value &_collection;
};

/**
 * Represents a single field of a given collection's elements.
 */
struct FieldContent : public Content {
  FieldContent(Content &parent, unsigned field_index)
    : Content(ContentKind::CONTENT_FIELD),
      _parent(parent),
      _field_index(field_index) {}

  std::string to_string() const override {
    return "field(" + this->_parent.to_string() + ", "
           + std::to_string(this->_field_index) + ")";
  }

  bool operator==(Content &other) const override {
    auto *other_field = llvm::memoir::dyn_cast<FieldContent>(&other);
    if (not other_field) {
      return false;
    }

    return (this->_field_index == other_field->_field_index)
           and (this->_parent == other_field->_parent);
  }

  Content &substitute(Content &from, Content &to) override {
    auto &subst_parent = this->_parent.substitute(from, to);
    if (&subst_parent == &this->_parent) {
      return *this;
    }
    return Content::create<FieldContent>(subst_parent, this->_field_index);
  }

  static bool classof(const Content *content) {
    return content->kind() == ContentKind::CONTENT_FIELD;
  }

  Content &parent() {
    return this->_parent;
  }

  unsigned field_index() {
    return this->_field_index;
  }

protected:
  Content &_parent;
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

  std::string to_string() const override {
    return "(" + this->_content.to_string() + " | " + this->_lhs.to_string()
           + " " + llvm::CmpInst::getPredicateName(this->_predicate).str() + " "
           + this->_rhs.to_string() + ")";
  }

  bool operator==(Content &other) const override {
    auto *other_cond = llvm::memoir::dyn_cast<ConditionalContent>(&other);
    if (not other_cond) {
      return false;
    }

    return (this->_predicate == other_cond->_predicate)
           and (this->_lhs == other_cond->_lhs)
           and (this->_rhs == other_cond->_rhs)
           and (this->_content == other_cond->_content);
  }

  Content &substitute(Content &from, Content &to) override {
    auto &subst_content = this->_content.substitute(from, to);
    auto &subst_lhs = this->_lhs.substitute(from, to);
    auto &subst_rhs = this->_rhs.substitute(from, to);
    if (&subst_content == &this->_content and &subst_lhs == &this->_lhs
        and &subst_rhs == &this->_rhs) {
      return *this;
    }
    return Content::create<ConditionalContent>(subst_content,
                                               this->_predicate,
                                               subst_lhs,
                                               subst_rhs);
  }

  bool same_conditional(const ConditionalContent &other) const {
    return (this->_predicate == other._predicate) and (this->_lhs == other._lhs)
           and (this->_rhs == other._rhs);
  }

  static bool classof(const Content *content) {
    return content->kind() == ContentKind::CONTENT_CONDITIONAL;
  }

  Content &content() {
    return this->_content;
  }

  llvm::CmpInst::Predicate predicate() {
    return this->_predicate;
  }

  Content &lhs() {
    return this->_lhs;
  }

  Content &rhs() {
    return this->_rhs;
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

  TupleContent(const llvm::memoir::vector<Content *> &elements)
    : Content(ContentKind::CONTENT_TUPLE),
      _elements(elements) {}

  std::string to_string() const override {
    return "[ "
           + std::accumulate(std::next(this->_elements.begin()),
                             this->_elements.end(),
                             _elements.at(0)->to_string(),
                             [](std::string a, Content *c) {
                               return a + ", " + c->to_string();
                             })
           + " ]";
  }

  bool operator==(Content &other) const override {
    auto *other_tuple = llvm::memoir::dyn_cast<TupleContent>(&other);
    if (not other_tuple) {
      return false;
    }

    if (this->_elements.size() != other_tuple->_elements.size()) {
      return false;
    }

    for (size_t i = 0; i < this->_elements.size(); ++i) {
      auto *this_field = this->_elements[i];
      auto *other_field = other_tuple->_elements[i];
      if (*this_field != *other_field) {
        return false;
      }
    }

    return true;
  }

  Content &substitute(Content &from, Content &to) override {

    bool modified = false;

    llvm::memoir::vector<Content *> subst_elements = {};
    subst_elements.reserve(this->_elements.size());

    for (auto *elem : this->_elements) {
      auto &subst_elem = elem->substitute(from, to);
      if (&subst_elem != elem) {
        modified = true;
      }
      subst_elements.push_back(&subst_elem);
    }

    if (modified) {
      return Content::create<TupleContent>(subst_elements);
    } else {
      return *this;
    }
  }

  static bool classof(const Content *content) {
    return content->kind() == ContentKind::CONTENT_TUPLE;
  }

  llvm::memoir::vector<Content *> &elements() {
    return this->_elements;
  }

  Content &element(size_t index) {
    return *(this->_elements[index]);
  }

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

  std::string to_string() const override {
    return "( " + this->_lhs.to_string() + " ∪ " + this->_rhs.to_string()
           + " )";
  }

  bool operator==(Content &other) const override {
    auto *other_union = llvm::memoir::dyn_cast<UnionContent>(&other);
    if (not other_union) {
      return false;
    }
    return (this->_lhs == other_union->_lhs)
           and (this->_rhs == other_union->_rhs);
  }

  Content &substitute(Content &from, Content &to) override {
    auto &subst_lhs = this->_lhs.substitute(from, to);
    auto &subst_rhs = this->_rhs.substitute(from, to);

    if (&subst_lhs == &this->_lhs and &subst_rhs == &this->_rhs) {
      return *this;
    }

    return Content::create<UnionContent>(subst_lhs, subst_rhs);
  }

  static bool classof(const Content *content) {
    return content->kind() == ContentKind::CONTENT_UNION;
  }

  Content &lhs() {
    return this->_lhs;
  }

  Content &rhs() {
    return this->_rhs;
  }

protected:
  Content &_lhs;
  Content &_rhs;
};

} // namespace folio
