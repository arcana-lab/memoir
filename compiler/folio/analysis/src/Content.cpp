#include "folio/analysis/Content.hpp"

#include "memoir/support/Print.hpp"

using namespace llvm::memoir;

namespace folio {

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

Content &KeyContent::substitute(Content &from, Content &to) {
  auto &subst = this->_collection.substitute(from, to);
  if (&subst == &this->_collection) {
    return *this;
  }
  return Content::create<KeyContent>(subst);
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

Content &ElementContent::substitute(Content &from, Content &to) {
  auto &subst = this->_collection.substitute(from, to);
  if (&subst == &this->_collection) {
    return *this;
  }
  return Content::create<ElementContent>(subst);
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

Content &UnionContent::substitute(Content &from, Content &to) {
  auto &subst_lhs = this->_lhs.substitute(from, to);
  auto &subst_rhs = this->_rhs.substitute(from, to);

  if (&subst_lhs == &this->_lhs and &subst_rhs == &this->_rhs) {
    return *this;
  }

  return Content::create<UnionContent>(subst_lhs, subst_rhs);
}

} // namespace folio
