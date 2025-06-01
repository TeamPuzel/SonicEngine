// Created by Lua (TeamPuzel) on May 25th 2025.
// Copyright (c) 2025 All rights reserved.
#pragma once
#include "Primitive.hpp"
#include <type_traits>

namespace core::match {
    /// The representation of match branches.
    template <typename T, typename U> struct Branch final {
        T pattern;
        U ret;
    };

    // template <typename T, typename U> concept Matchable = requires(T a, U b) {
    //     { a >> b } -> Same<Branch<T, U>>;
    //     { b == a } -> Same<Bool>;
    // };

    /// A type trait identifying types usable in a match expression.
    template <typename T, typename U, typename = void> struct Matchable : std::false_type {};
    template <typename T, typename U>
    struct Matchable<
        T, U,
        std::enable_if_t<
            std::is_same_v<decltype(std::declval<T const&>() >> std::declval<U const&>()), Branch<T, U>> and
            std::is_same_v<decltype(std::declval<U const&>() == std::declval<T const&>()), bool>>> : std::true_type {};

    template <typename T, typename U, typename V, usize const N>
    // std::enable_if_t<IsEquatable<T>::value and IsMatchable<U, T>::value, Int> = 0>
    constexpr auto match(T value, Branch<U, V> const (&branches)[N], V default_v) -> V {
        for (auto const& branch : branches)
            if (branch.pattern == value)
                return branch.ret;
        return default_v;
    }
}
