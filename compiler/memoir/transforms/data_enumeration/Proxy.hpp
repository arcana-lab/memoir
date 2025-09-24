#ifndef FOLIO_TRANSFORMS_PROXY_H
#define FOLIO_TRANSFORMS_PROXY_H

#include "llvm/IR/Value.h"

#include "Content.hpp"

namespace memoir {

enum ProxyKind { PROXY_NATURAL, PROXY_ARTIFICIAL };

/**
 * A Proxy, which represents the ability to proxy one collection with another.
 */
struct Proxy {
public:
  /**
   * Get the collection being proxied.
   */
  Content &proxied() {
    return this->_proxied;
  }

  ProxyKind kind() const {
    return this->_kind;
  }

  Proxy(ProxyKind kind, Content &proxied) : _kind(kind), _proxied(proxied) {}

  virtual ~Proxy() {}

protected:
  ProxyKind _kind;
  Content &_proxied;
};

/**
 * A _natural_ proxy, where the proxy exists in the program.
 */
struct NaturalProxy : public Proxy {
public:
  /**
   * Get the definition of the existing proxy collection.
   */
  llvm::Value &proxy() {
    return this->_proxy;
  }

  static bool classof(const Proxy *p) {
    return p->kind() == ProxyKind::PROXY_NATURAL;
  }

  NaturalProxy(Content &proxied, llvm::Value &proxy)
    : Proxy(ProxyKind::PROXY_NATURAL, proxied),
      _proxy(proxy) {}

protected:
  llvm::Value &_proxy;
};

/**
 * An _artifical_ proxy, where the program can be transformed to introduce a new
 * proxy colection.
 */
struct ArtificialProxy : public Proxy {
public:
  ArtificialProxy(Content &proxied)
    : Proxy(ProxyKind::PROXY_ARTIFICIAL, proxied) {}

  static bool classof(const Proxy *p) {
    return p->kind() == ProxyKind::PROXY_ARTIFICIAL;
  }
};

} // namespace memoir

#endif // FOLIO_TRANSFORMS_PROXY_H
