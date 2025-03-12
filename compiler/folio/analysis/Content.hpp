#ifndef FOLIO_ANALYSIS_CONTENT_H
#define FOLIO_ANALYSIS_CONTENT_H

#include <numeric>
#include <string>

#include "llvm/Support/raw_ostream.h"

#include "memoir/ir/Types.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

namespace folio {

enum ContentKind {
  CONTENT_UNDERDEFINED,
  CONTENT_EMPTY,
  CONTENT_STRUCT,
  CONTENT_SCALAR,
  CONTENT_KEYS,
  CONTENT_RANGE,
  CONTENT_ELEMENTS,
  CONTENT_FIELD,
  CONTENT_SUBSET,
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
    return C::create(std::forward<Args>(args)...);
  }

  Content(ContentKind kind) : _kind(kind) {}

  virtual std::string to_string() const = 0;

  friend std::ostream &operator<<(std::ostream &os, const Content &C) {
    return os << C.to_string();
  }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const Content &C) {
    return os << C.to_string();
  }

  virtual bool operator==(Content &other) const = 0;

  bool operator!=(Content &other) const {
    return not(*this == other);
  }

  virtual Content &substitute(Content &from, Content &to) = 0;

  virtual ~Content() {}

  ContentKind kind() const {
    return _kind;
  }

  virtual llvm::memoir::Type *type() = 0;

protected:
  ContentKind _kind;
};

/**
 * Represents an underdefined content, this can be thought of as the tautology:
 *   X's contents are comprised of X's contents.
 */
struct UnderdefinedContent : public Content {
  UnderdefinedContent() : Content(ContentKind::CONTENT_UNDERDEFINED) {}

  static Content &create();

  std::string to_string() const override {
    return "⊥";
  }

  bool operator==(Content &other) const override {
    return llvm::memoir::isa<UnderdefinedContent>(&other);
  }

  Content &substitute(Content &from, Content &to) override;

  llvm::memoir::Type *type() override {
    return nullptr;
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

  static Content &create();

  std::string to_string() const override {
    return "∅";
  }

  bool operator==(Content &other) const override {
    return llvm::memoir::isa<EmptyContent>(&other);
  }

  Content &substitute(Content &from, Content &to) override;

  llvm::memoir::Type *type() override {
    return nullptr;
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

  static Content &create(llvm::Value &value);

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

  Content &substitute(Content &from, Content &to) override;

  static bool classof(const Content *content) {
    return content->kind() == ContentKind::CONTENT_STRUCT;
  }

  llvm::Value &value() {
    return this->_value;
  }

  llvm::memoir::Type *type() override {
    return llvm::memoir::type_of(this->_value);
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

  static Content &create(llvm::Value &value);

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

  Content &substitute(Content &from, Content &to) override;

  static bool classof(const Content *content) {
    return content->kind() == ContentKind::CONTENT_SCALAR;
  }

  llvm::Value &value() {
    return this->_value;
  }

  llvm::memoir::Type *type() override {
    return llvm::memoir::type_of(this->_value);
  }

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

  static Content &create(llvm::Value &collection);

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

  Content &substitute(Content &from, Content &to) override;

  static bool classof(const Content *content) {
    return content->kind() == ContentKind::CONTENT_KEYS;
  }

  llvm::Value &collection() {
    return this->_collection;
  }

  llvm::memoir::Type *type() override {
    auto *type = llvm::memoir::type_of(this->collection());
    if (auto *assoc_type =
            llvm::memoir::dyn_cast<llvm::memoir::AssocArrayType>(type)) {
      return &assoc_type->getKeyType();
    }

    auto &collection = this->collection();
    const llvm::DataLayout *data_layout = nullptr;
    if (auto *inst = llvm::memoir::dyn_cast<llvm::Instruction>(&collection)) {
      data_layout = &inst->getModule()->getDataLayout();
    } else if (auto *arg =
                   llvm::memoir::dyn_cast<llvm::Argument>(&collection)) {
      data_layout = &arg->getParent()->getParent()->getDataLayout();
    }

    return &llvm::memoir::Type::get_size_type(*data_layout);
  }

protected:
  llvm::Value &_collection;
};

/**
 * Represents a contiguous range with size equal to the given collection.
 */
struct RangeContent : public Content {
  RangeContent(llvm::Value &collection)
    : Content(ContentKind::CONTENT_RANGE),
      _collection(collection) {}

  static Content &create(llvm::Value &collection);

  std::string to_string() const override {
    return "range(" + llvm::memoir::value_name(this->_collection) + ")";
  }

  bool operator==(Content &other) const override {
    auto *other_range = llvm::memoir::dyn_cast<RangeContent>(&other);
    if (not other_range) {
      return false;
    }

    return &this->_collection == &other_range->_collection;
  }

  Content &substitute(Content &from, Content &to) override;

  static bool classof(const Content *content) {
    return content->kind() == ContentKind::CONTENT_RANGE;
  }

  llvm::Value &collection() {
    return this->_collection;
  }

  llvm::memoir::Type *type() override {
    auto &collection = this->collection();

    const llvm::DataLayout *data_layout = nullptr;
    if (auto *inst = llvm::memoir::dyn_cast<llvm::Instruction>(&collection)) {
      data_layout = &inst->getModule()->getDataLayout();
    } else if (auto *arg = llvm::memoir::dyn_cast<llvm::Argument>(inst)) {
      data_layout = &arg->getParent()->getParent()->getDataLayout();
    }

    return &llvm::memoir::Type::get_size_type(*data_layout);
  }

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

  static Content &create(llvm::Value &collection);

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

  Content &substitute(Content &from, Content &to) override;

  static bool classof(const Content *content) {
    return content->kind() == ContentKind::CONTENT_ELEMENTS;
  }

  llvm::Value &collection() {
    return this->_collection;
  }

  llvm::memoir::Type *type() override {
    auto *type = llvm::memoir::type_of(this->collection());
    auto *collection_type =
        llvm::memoir::cast<llvm::memoir::CollectionType>(type);
    auto &element_type = collection_type->getElementType();
    return &element_type;
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

  static Content &create(Content &parent, unsigned field_index);

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

  Content &substitute(Content &from, Content &to) override;

  static bool classof(const Content *content) {
    return content->kind() == ContentKind::CONTENT_FIELD;
  }

  Content &parent() {
    return this->_parent;
  }

  unsigned field_index() {
    return this->_field_index;
  }

  llvm::memoir::Type *type() override {
    auto *parent_type =
        llvm::memoir::cast<llvm::memoir::TupleType>(this->parent().type());
    return &parent_type->getFieldType(this->field_index());
  }

protected:
  Content &_parent;
  unsigned _field_index;
};

/**
 * Represents a subset of the nested content.
 */
struct SubsetContent : public Content {
public:
  SubsetContent(Content &content)
    : Content(ContentKind::CONTENT_SUBSET),
      _content(content) {}

  static Content &create(Content &content);

  std::string to_string() const override {
    return "subsetof(" + this->_content.to_string() + ")";
  }

  bool operator==(Content &other) const override {
    auto *other_subset = llvm::memoir::dyn_cast<SubsetContent>(&other);
    if (not other_subset) {
      return false;
    }

    return (this->_content == other_subset->_content);
  }

  Content &substitute(Content &from, Content &to) override;

  static bool classof(const Content *content) {
    return content->kind() == ContentKind::CONTENT_SUBSET;
  }

  Content &content() {
    return this->_content;
  }

  llvm::memoir::Type *type() override {
    return this->content().type();
  }

protected:
  Content &_content;
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

  static Content &create(Content &content,
                         llvm::CmpInst::Predicate pred,
                         Content &lhs,
                         Content &rhs);

  std::string to_string() const override {
    return "[" + this->_content.to_string() + " | " + this->_lhs.to_string()
           + " " + llvm::CmpInst::getPredicateName(this->_predicate).str() + " "
           + this->_rhs.to_string() + "]";
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

  Content &substitute(Content &from, Content &to) override;

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

  llvm::memoir::Type *type() override {
    return this->content().type();
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

  static Content &create(std::initializer_list<Content *> elements);

  static Content &create(const llvm::memoir::vector<Content *> &elements);

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

  Content &substitute(Content &from, Content &to) override;

  static bool classof(const Content *content) {
    return content->kind() == ContentKind::CONTENT_TUPLE;
  }

  llvm::memoir::vector<Content *> &elements() {
    return this->_elements;
  }

  Content &element(size_t index) {
    return *(this->_elements[index]);
  }

  llvm::memoir::Type *type() override {
    MEMOIR_UNREACHABLE("Type of TupleContent is unimplemented!");
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

  static Content &create(Content &lhs, Content &rhs);

  std::string to_string() const override {
    return "( " + this->_lhs.to_string() + " ∪ " + this->_rhs.to_string()
           + " )";
  }

  bool operator==(Content &other) const override {
    auto *other_union = llvm::memoir::dyn_cast<UnionContent>(&other);
    if (not other_union) {
      return false;
    }
    return ((this->_lhs == other_union->_lhs)
            and (this->_rhs == other_union->_rhs))
           or ((this->_lhs == other_union->_rhs)
               and (this->_rhs == other_union->_lhs));
  }

  Content &substitute(Content &from, Content &to) override;

  static bool classof(const Content *content) {
    return content->kind() == ContentKind::CONTENT_UNION;
  }

  Content &lhs() {
    return this->_lhs;
  }

  Content &rhs() {
    return this->_rhs;
  }

  llvm::memoir::Type *type() override {
    auto *lhs_type = this->lhs().type();
    auto *rhs_type = this->rhs().type();
    if (not lhs_type) {
      return rhs_type;
    } else if (not rhs_type) {
      return lhs_type;
    } else if (lhs_type == rhs_type) {
      return lhs_type;
    } else {
      MEMOIR_UNREACHABLE("Cannot unify type of UnionContent!");
    }
  }

protected:
  Content &_lhs;
  Content &_rhs;
};

using ContentSummary = typename std::pair<Content *, Content *>;
using Contents = typename llvm::memoir::map<llvm::Value *, ContentSummary>;

} // namespace folio

#endif // FOLIO_ANALYSIS_CONTENT_H
