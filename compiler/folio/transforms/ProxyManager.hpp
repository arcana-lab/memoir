#ifndef FOLIO_TRANSFORMS_PROXYMANAGER_H
#define FOLIO_TRANSFORMS_PROXYMANAGER_H

#include <optional>

#include "memoir/support/DataTypes.hpp"

#include "folio/analysis/Content.hpp"
#include "folio/transforms/Proxy.hpp"

namespace folio {

class ProxyManager {
public:
  /**
   * Initialize the singleton ProxyManager instance.
   */
  static void initialize(Contents &contents);

  /**
   * Queries the manager to see if a natural proxy exists under the given
   * conditions.
   *
   * @param content the content that need be proxied.
   * @param uses the set of uses that the natural proxy must provide for.
   * @returns the natural proxy, or NULL if one does not exist.
   */
  static Proxy *has_natural_proxy(Content &content,
                                  llvm::memoir::Set<llvm::Use *> &uses);

  /**
   * Requests an artificial proxy of the given content.
   *
   * @param content the content that need be proxied.
   * @returns an artificial proxy of the content, or NULL if one cannot be
   * created.
   */
  static Proxy *request(Content &content);

  /**
   * Get a reference to the list of proxies.
   */
  static llvm::memoir::List<Proxy *> proxies() {
    return ProxyManager::_manager->_proxies;
  }

  /**
   * Get a unique identifier for the given proxy.
   */
  static uint64_t get_id(const Proxy &proxy) {
    uint64_t id = 0;
    for (auto *other : ProxyManager::_manager->proxies()) {
      if (other == &proxy) {
        return id;
      }

      ++id;
    }

    MEMOIR_UNREACHABLE("Could not find the given proxy in the manager!");
  }

protected:
  // Singleton
  ProxyManager(Contents &contents) : _contents(contents) {}
  static ProxyManager *_manager;

  // Methods.
  Proxy *_has_natural_proxy(Content &content,
                            llvm::memoir::Set<llvm::Use *> &uses);
  Proxy *_request(Content &content);

  // Owned state.
  llvm::memoir::List<Proxy *> _proxies;

  // Borrowed state.
  Contents &_contents;
};

} // namespace folio

#endif // FOLIO_TRANSFORMS_PROXYMANAGER_H
