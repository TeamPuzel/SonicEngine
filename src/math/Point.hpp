// Created by Lua (TeamPuzel) on July 30th 2025.
// Copyright (c) 2025 All rights reserved.
//
// This header defines point arithmetic used by the game.
#pragma once
#include <primitive>

namespace math {
    template <typename T> struct [[clang::trivial_abi]] point final { // NOLINT(readability-identifier-naming)
        T x, y;
    };
}
