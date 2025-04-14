#include "folio/analysis/Content.hpp"
#include "folio/analysis/ContentSimplification.hpp"

#include "memoir/support/Print.hpp"

using namespace llvm::memoir;

namespace folio {

// Creation.
Content &UnderdefinedContent::create() {
  return MEMOIR_SANITIZE(new UnderdefinedContent(),
                         "Failed to create UnderdefinedContent.");
}

Content &EmptyContent::create() {
  return MEMOIR_SANITIZE(new EmptyContent(), "Failed to create EmptyContent.");
}

Content &StructContent::create(llvm::Value &V) {
  return MEMOIR_SANITIZE(new StructContent(V),
                         "Failed to create StructContent.");
}

Content &ScalarContent::create(llvm::Value &V) {
  return MEMOIR_SANITIZE(new ScalarContent(V),
                         "Failed to create ScalarContent.");
}

Content &KeysContent::create(llvm::Value &V) {
  return MEMOIR_SANITIZE(new KeysContent(V), "Failed to create KeysContent.");
}

Content &ElementsContent::create(llvm::Value &V) {
  return MEMOIR_SANITIZE(new ElementsContent(V),
                         "Failed to create ElementsContent.");
}

Content &RangeContent::create(llvm::Value &V) {
  return MEMOIR_SANITIZE(new RangeContent(V), "Failed to create RangeContent.");
}

Content &SubsetContent::create(Content &C) {
  return MEMOIR_SANITIZE(new SubsetContent(simplify(C)),
                         "Failed to create SubsetContent.");
}

Content &FieldContent::create(Content &parent, unsigned field_index) {
  return MEMOIR_SANITIZE(new FieldContent(simplify(parent), field_index),
                         "Failed to create FieldContent.");
}

Content &ConditionalContent::create(Content &C,
                                    llvm::CmpInst::Predicate pred,
                                    Content &lhs,
                                    Content &rhs) {
  return MEMOIR_SANITIZE(new ConditionalContent(simplify(C), pred, lhs, rhs),
                         "Failed to create ConditionalContent.");
}

Content &TupleContent::create(const Vector<Content *> &elements) {
  auto &tuple = MEMOIR_SANITIZE(new TupleContent(elements),
                                "Failed to create TupleContent.");
  std::for_each(tuple.elements().begin(),
                tuple.elements().end(),
                [](auto &elem) { elem = &simplify(*elem); });

  return tuple;
}

Content &TupleContent::create(std::initializer_list<Content *> elements) {
  return MEMOIR_SANITIZE(
      new TupleContent(Vector<Content *>(
          std::forward<std::initializer_list<Content *>>(elements))),
      "Failed to create TupleContent.");
}

Content &UnionContent::create(Content &lhs, Content &rhs) {
  return simplify(
      MEMOIR_SANITIZE(new UnionContent(simplify(lhs), simplify(rhs)),
                      "Failed to create UnionContent."));
}

// Substitution.
Content &UnderdefinedContent::substitute(Content &from, Content &to) {
  return *this;
}

Content &EmptyContent::substitute(Content &from, Content &to) {
  return *this;
}

Content &StructContent::substitute(Content &from, Content &to) {
  if (from == *this) {
    return to;
  }
  return *this;
}

Content &ScalarContent::substitute(Content &from, Content &to) {
  if (from == *this) {
    return to;
  }
  return *this;
}

Content &KeysContent::substitute(Content &from, Content &to) {
  if (from == *this) {
    return to;
  }
  return *this;
}

Content &RangeContent::substitute(Content &from, Content &to) {
  if (from == *this) {
    return to;
  }
  return *this;
}

Content &ElementsContent::substitute(Content &from, Content &to) {
  if (from == *this) {
    return to;
  }
  return *this;
}

Content &FieldContent::substitute(Content &from, Content &to) {
  auto &subst_parent = this->_parent.substitute(from, to);
  if (&subst_parent == &this->_parent) {
    return *this;
  }
  return Content::create<FieldContent>(subst_parent, this->_field_index);
}

Content &SubsetContent::substitute(Content &from, Content &to) {
  auto &subst = this->content().substitute(from, to);
  if (&subst == &this->content()) {
    return *this;
  }
  return Content::create<SubsetContent>(subst);
}

Content &ConditionalContent::substitute(Content &from, Content &to) {
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

Content &TupleContent::substitute(Content &from, Content &to) {

  bool modified = false;

  llvm::memoir::Vector<Content *> subst_elements = {};
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

Content &UnionContent::substitute(Content &from, Content &to) {
  auto &subst_lhs = this->_lhs.substitute(from, to);
  auto &subst_rhs = this->_rhs.substitute(from, to);

  if (&subst_lhs == &this->_lhs and &subst_rhs == &this->_rhs) {
    return *this;
  }

  return Content::create<UnionContent>(subst_lhs, subst_rhs);
}

} // namespace folio
