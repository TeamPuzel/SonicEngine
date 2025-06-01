#pragma once
#include "Primitive.hpp"
#include <type_traits>

namespace core {
    template <typename T>
    constexpr auto normalize(T n, T min_from, T max_from, T min_to, T max_to) -> T
        requires std::is_floating_point_v<T>
    {
        return (max_to - min_to) / (max_from - min_from) * (n - max_from) + max_to;
    }

    template <typename T, usize const N> class Vector final {
        T data[N];
    };
}
