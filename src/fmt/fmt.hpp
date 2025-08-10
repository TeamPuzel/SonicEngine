// Created by Lua (TeamPuzel) on August 10th 2025.
// Copyright (c) 2025 All rights reserved.
#pragma once
#include <string>
#include <sstream>

namespace fmt {
    template <typename T> auto display(T const& value, std::stringstream& fmt);

    template <typename... Args> auto format(char const* fmt, Args... args) -> std::string {
        std::stringstream ret;

        return ret.str();
    }
}
