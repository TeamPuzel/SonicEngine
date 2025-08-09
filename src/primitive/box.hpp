// Created by Lua (TeamPuzel) on August 3rd 2025.
// Copyright (c) 2025 All rights reserved.
//
// An owned allocation.
// It was either this, violating exception safety or inheriting from std::vector and that's just plain stupid.
#pragma once
#include <utility>

template <typename T> class box final { // NOLINT(readability-identifier-naming)
    T* inner;

    explicit box(T* inner) noexcept : inner(inner) {}

  public:
    box() noexcept : inner(nullptr) {}

    template <typename U> friend class box;

    template <typename... Args> static auto make(Args&&...args) -> box {
        return box(new T(std::forward<Args>(args)...));
    }

    box(box const&) = delete;
    auto operator=(box const&) -> box& = delete;

    template <typename U, std::enable_if_t<std::is_convertible_v<U*, T*>, int> = 0>
    box(box<U>&& existing) noexcept : inner(existing.inner) {
        existing.inner = nullptr;
    }

    template <typename U, std::enable_if_t<std::is_convertible_v<U*, T*>, int> = 0>
    auto operator=(box<U>&& existing) noexcept -> box& {
        if ((void*)this != (void*)&existing) {
            delete this->inner;
            this->inner = existing.inner;
            existing.inner = nullptr;
        }
        return *this;
    }

    ~box() noexcept {
        delete this->inner; // Deleting a nullptr is sound.
    }

    // // Handle variance.
    // template <typename U, std::enable_if_t<std::is_convertible_v<T*, U*>, int> = 0> operator box<U>() && noexcept {
    //     auto base_ptr = this->inner;
    //     this->inner = nullptr;
    //     return box(base_ptr);
    // }

    auto operator->() const noexcept [[clang::lifetimebound]] -> T* {
        return this->inner;
    }

    auto operator*() const noexcept [[clang::lifetimebound]] -> T& {
        return *this->inner;
    }

    auto raw() const noexcept [[clang::lifetimebound]] -> T* {
        return this->inner;
    }
};
