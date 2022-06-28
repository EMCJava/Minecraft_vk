//
// Created by samsa on 6/27/2022.
//

#ifndef MINECRAFT_VK_UTILITY_MATH_MATH_HPP
#define MINECRAFT_VK_UTILITY_MATH_MATH_HPP

#include <concepts>

template <std::integral auto Val, std::integral auto Base, decltype(Val) Power = 0, typename Ty = decltype(Val)>
struct IntLog {

    static constexpr Ty value { IntLog<Val / Base, Base, Power + 1, Ty>::value };   // or also decltype(Val) t {Val};
};

template <std::integral auto Base, auto Power, typename Ty>
struct IntLog<1, Base, Power, Ty> {

    static constexpr Ty value { Power };   // or also decltype(Val) t {Val};};
};

#endif   // MINECRAFT_VK_UTILITY_MATH_MATH_HPP
