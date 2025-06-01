// Created by Lua (TeamPuzel) on May 25th 2025.
// Copyright (c) 2025 All rights reserved.
#pragma once
#include "Primitive.hpp"
#include <type_traits>
#include <utility>

namespace core {
    /// Toggles a boolean in place.
    constexpr void toggle(bool& value) {
        value = !value;
    }

    template <typename T>
    [[nodiscard]] constexpr auto move([[clang::lifetimebound]] T&& value) -> std::remove_reference_t<T>&& {
        using U = std::remove_reference_t<T>;
        return static_cast<U&&>(value);
    }

    template <typename T>
    [[nodiscard]] constexpr auto forward([[clang::lifetimebound]] std::remove_reference_t<T>& value) -> T&& {
        return static_cast<T&&>(value);
    }

    template <typename T>
    [[nodiscard]] constexpr auto forward([[clang::lifetimebound]] std::remove_reference_t<T>&& value) -> T&& {
        static_assert(not std::is_lvalue_reference_v<T>, "Cannot forward an rvalue as an lvalue");
        return static_cast<T&&>(value);
    }

    /// A trivial wrapper which stores a non-trivial type T but does not automatically destroy it or otherwise
    /// preserve its copyability semantics. It is an unsafe construct which can be used to optimize away
    /// constructor and destructor calls when storing temporary moved from objects.
    ///
    /// This type is very unsafe.
    template <typename T> class Unmanaged final {
        /// An anonymous union which for some reason is the only way in C++ to bypass type semantics.
        union {
            T inner;
        };

      public:
        constexpr Unmanaged() {}
        template <typename U> constexpr Unmanaged(U&& inner) : inner(std::forward<T>(inner)) {}

        /// Manually destroy the stored object.
        constexpr void destroy() {
            inner.~T();
        }

        /// Take the stored object and manage it.
        constexpr auto take() -> T {
            return std::move(inner);
        }

        constexpr auto operator*() const -> T const& {
            return inner;
        }

        constexpr auto operator*() -> T& {
            return inner;
        }

        constexpr auto operator->() const -> T const& {
            return inner;
        }

        constexpr auto operator->() -> T& {
            return inner;
        }
    };

    /// Unsafely bitwise copy raw memory.
    constexpr auto memcopy(void* src, void* dst, usize count) -> void {
        auto const s = (u8*)src, d = (u8*)dst;
        for (usize i = 0; i < count; i += 1)
            d[i] = s[i];
    }

    /// Unsafely bitwise copy non-trivial types. Similar to memcopy but it infers the size from the source.
    ///
    /// In C++ it is sound to cast objects to unsigned bytes.
    template <typename T> constexpr auto copy(T* src, void* dst) -> void {
        auto const s = (u8*)src, d = (u8*)dst;
        for (usize i = 0; i < sizeof(T); i += 1)
            d[i] = s[i];
    }

    /// Unsafely bitwise extract a non-trivial value from a memory address.
    template <typename T> constexpr auto read(T* ptr) -> Unmanaged<T> {
        Unmanaged<T> temp;
        core::copy(ptr, &temp);
        return temp;
    }

    /// Unsafely bitwise store a non-trivial value to a memory address.
    template <typename T> constexpr void write(T* ptr, Unmanaged<T>& value) {
        core::copy(&value, ptr);
    }

    /// Unsafely bitwise store a non-trivial value to a memory address.
    template <typename T> constexpr void write(T* ptr, Unmanaged<T>&& value) {
        core::copy(&value, ptr);
    }

    /// A swap function which avoids invoking any constructors or destructors.
    template <typename T> constexpr void swap(T& a, T& b) {
        Unmanaged<T> temp = core::read(&a);
        core::write(&a, core::read(&b));
        core::write(&b, temp);
    }

    template <typename T, typename U> constexpr auto replace([[clang::lifetimebound]] T& a, U&& b) -> T {
        T old = core::move(a);
        a = core::forward<U>(b);
        return old;
    }

    [[noreturn]] inline void panic(char const* message = nullptr) {
        // TODO(?): Output trace (probably unreasonable)
        // TODO(!): Output message
        __builtin_trap();
    }

    [[noreturn]] inline void todo(char const* message = "not implemented") {
        panic(message);
    }

    /// Guards internal invariants.
    [[clang::always_inline]] inline void assert(bool condition, char const* message = "assertion failure") {
#ifdef DEBUG
        if (not condition)
            panic(message);
#endif
    }

    /// Guards external invariants.
    [[clang::always_inline]] inline void precondition(bool condition, char const* message = "precondition failure") {
#ifndef UNCHECKED_PRECONDITION
        if (not condition)
            panic(message);
#endif
    }
}
