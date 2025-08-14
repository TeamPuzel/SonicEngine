// Created by Lua (TeamPuzel) on August 13th 2025.
// Copyright (c) 2025 All rights reserved.
#pragma once
#include <primitive>
#include <string_view>
#include <vector>

/// Encapsulates all global side effects.
///
/// Loosely inspired by Zig's idea. Has the pleasant quality in how, similarly to an IO monad, it
/// indicates function purity. You know that functions that don't take Io& can't perform any.
/// You know that functions which are pure and don't take Io& can't call other Io& functions.
///
/// Unlike an IO monad it's easy to integrate, it's just a context.
///
/// Io itself is an abstract class and requires a concrete platform implementation to be used.
class Io {
  public:
    class Error {
      protected:
        constexpr Error() {}

      public:
        Error(Error const&) = delete;
        Error(Error&&) = delete;
        auto operator=(Error const&) -> Error& = delete;
        auto operator=(Error&&) -> Error& = delete;
        virtual ~Error() noexcept {}
    };

  protected:
    Io() noexcept {}

    virtual auto perform_read_file(char const* path) -> std::vector<u8> = 0;
    virtual auto perform_open_library(char const* path) -> void* = 0;
    virtual void perform_close_library(void* library) = 0;
    virtual auto perform_load_symbol(void* library, char const* name) -> void* = 0;

  public:
    Io(Io const&) = delete;
    Io(Io&&) = delete;
    auto operator=(Io const&) -> Io& = delete;
    auto operator=(Io&&) -> Io& = delete;

    virtual ~Io() noexcept {}

    class DynamicLibrary final {
        Io& io;
        void* obj;

        DynamicLibrary(Io& io, void* obj) : io(io), obj(obj) {}

        friend class Io;

      public:
        DynamicLibrary(DynamicLibrary const&) = delete;
        auto operator=(DynamicLibrary const&) -> DynamicLibrary& = delete;

        DynamicLibrary(DynamicLibrary&& other) noexcept : io(other.io), obj(other.obj) {
            other.obj = nullptr;
        }

        auto operator=(DynamicLibrary&& other) noexcept -> DynamicLibrary& {
            if (this != &other) {
                if (obj) io.perform_close_library(obj);
                obj = other.obj;
                other.obj = nullptr;
            }
            return *this;
        }

        ~DynamicLibrary() noexcept {
            if (obj) {
                io.perform_close_library(obj);
                obj = nullptr;
            }
        }

        auto symbol(std::string_view name) const -> void* {
            return io.perform_load_symbol(obj, name.data());
        }
    };

    auto open_library(std::string_view path) [[clang::lifetimebound]] -> DynamicLibrary {
        return DynamicLibrary(*this, perform_open_library(path.data()));
    }

    auto read_file(std::string_view path) -> std::vector<u8> {
        return perform_read_file(path.data());
    }
};
