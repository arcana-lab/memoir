#ifndef MEMOIR_SUPPORT_TYPETRAITS_H
#define MEMOIR_SUPPORT_TYPETRAITS_H

#include <type_traits>

template <class T, template <class...> class Template>
inline constexpr bool is_instance_of_v = std::false_type{};

template <template <class...> class Template, class... Args>
inline constexpr bool is_instance_of_v<Template<Args...>, Template> =
    std::true_type{};

#endif // MEMOIR_SUPPORT_TYPETRAITS_H
