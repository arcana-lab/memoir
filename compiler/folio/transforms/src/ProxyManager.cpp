#include "folio/transforms/ProxyManager.hpp"

using namespace llvm::memoir;

namespace folio {

void ProxyManager::initialize(Contents &contents) {
  // If a manager already exists, delete.
  if (ProxyManager::_manager) {
    delete ProxyManager::_manager;
  }

  // Construct a new manager.
  ProxyManager::_manager = new ProxyManager(contents);

  return;
}

Proxy *ProxyManager::has_natural_proxy(Content &content,
                                       Set<llvm::Use *> &uses) {
  MEMOIR_NULL_CHECK(ProxyManager::_manager,
                    "Using ProxyManager before calling initialize!");
  return ProxyManager::_manager->_has_natural_proxy(content, uses);
}

Proxy *ProxyManager::_has_natural_proxy(Content &content,
                                        Set<llvm::Use *> &uses) {
  // For a collection to be a natural proxy, it must:
  // 1. Be a sequence.
  // 2. Provide all elements necessary for the given content.
  // 3. Be available at all of the provided uses.

  // TODO.

  return nullptr;
}

Proxy *ProxyManager::request(Content &content) {
  MEMOIR_NULL_CHECK(ProxyManager::_manager,
                    "Using ProxyManager before calling initialize!");
  return ProxyManager::_manager->_request(content);
}

Proxy *ProxyManager::_request(Content &content) {
  // Check to see if an artificial proxy already exists for the given content.
  for (auto *proxy : this->_proxies) {
    if (proxy->proxied() == content) {
      return proxy;
    }
  }

  // If one does not already exist, construct a new artificial proxy for the
  // given content.
  auto *proxy = new ArtificialProxy(content);
  MEMOIR_NULL_CHECK(proxy, "Failed to create new ArtificialProxy");

  // Add the proxy to the list of proxies.
  this->_proxies.push_back(proxy);

  return proxy;
}

ProxyManager *ProxyManager::_manager = nullptr;

} // namespace folio
