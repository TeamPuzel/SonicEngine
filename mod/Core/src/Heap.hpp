// Created by Lua (TeamPuzel) on May 26th 2025.
// Copyright (c) 2025 All rights reserved.
#pragma once
#include "Primitive.hpp"
#include "Utility.hpp"

extern "C" {
    auto malloc(usize size) noexcept -> void*;
    void free(void* ptr) noexcept;
}

/// General allocation functions which depend on C's malloc and free.
namespace core::heap {
    /// Allocates memory for the type and panics on allocation failure.
    template <typename T> auto alloc() -> T* {
        auto const ptr = malloc(sizeof(T));
        core::precondition(ptr != nullptr);
        return (T*)ptr;
    }

    /// Allocates memory for an arbitrary amount of a certain type and panics on allocation failure.
    template <typename T> auto alloc(usize count) -> T* {
        auto const ptr = malloc(sizeof(T) * count);
        core::precondition(ptr != nullptr);
        return (T*)ptr;
    }

    /// Allocates memory for the type and returns nullptr on allocation failure.
    template <typename T> auto tryalloc() -> T* {
        auto const ptr = malloc(sizeof(T));
        return (T*)ptr;
    }

    /// Allocates memory for an arbitrary amount of a certain type and returns nullptr on allocation failure.
    template <typename T> auto tryalloc(usize count) -> T* {
        auto const ptr = malloc(sizeof(T) * count);
        return (T*)ptr;
    }

    /// Deallocates memory allocated using alloc or tryalloc.
    template <typename T> void dealloc(T* ptr) {
        free(ptr);
    }

    /// Allocates and constructs a type in place with the provided arguments, panics on allocation failure.
    template <typename T, typename... Args> auto create(Args&&... args) -> T* {
        auto const ptr = malloc(sizeof(T));
        core::precondition(ptr != nullptr);
        new (ptr) T(core::forward<Args>(args)...);
        return (T*)ptr;
    }

    /// Allocates and constructs a type in place with the provided arguments, returns nullptr on allocation failure.
    template <typename T, typename... Args> auto trycreate(Args&&... args) -> T* {
        auto const ptr = malloc(sizeof(T));
        if (not ptr)
            return nullptr;
        new (ptr) T(core::forward<Args>(args)...);
        return (T*)ptr;
    }

    /// Destroys objects allocated with create or trycreate.
    template <typename T> void destroy(T* ptr) {
        ptr->~T();
        free(ptr);
    }
}
