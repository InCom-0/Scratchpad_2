#pragma once

#include <array>
#include <meta>
#include <type_traits>


namespace incom::reflect {

enum class Color {
    Transparent,
    Red = 2,
    Green,
    Blue = 8,
    Yellow
};


template <typename E>
requires std::is_enum_v<E>
constexpr inline auto num_enumerators_of{std::meta::enumerators_of(^^E).size()};


template <typename E>
requires std::is_enum_v<E>
consteval auto get_enum_values() {
    std::array<E, num_enumerators_of<E>> res;

    template for (size_t i{}; constexpr auto &e : std::define_static_array(std::meta::enumerators_of(^^E))) {
        res[i++] = [:e:];
    }

    return res;
}


} // namespace incom::reflect