// Created by Lua (TeamPuzel) on August 13th 2025.
// Copyright (c) 2025 All rights reserved.
#pragma once
#include <primitive>
#include <string_view>
#include <vector>
#include <SDL3/SDL.h>

namespace rt::io {
    /// Loads the entirety of a file into program memory.
    static auto load(std::string_view path) -> std::vector<u8> {
        usize count;
        const auto data = (u8*) SDL_LoadFile(path.begin(), &count);
        auto ret = std::vector(data, data + count);
        SDL_free(data);
        return ret;
    }

    /// A dynamic library loader in terms of SDL3.
    /// It offers little control but it happens to make the sensible choice of RTLD_NOW | RTLD_LOCAL which is
    /// exactly what we want and I assume the semantics are preserved on other platforms.
    class DynamicLibrary final {
        SDL_SharedObject* obj;

        DynamicLibrary(SDL_SharedObject* obj) : obj(obj) {}

      public:
        static auto open(char const* path) -> DynamicLibrary {
            DynamicLibrary ret = SDL_LoadObject(path);
            if (not ret.obj) throw std::runtime_error(SDL_GetError());
            return ret;
        }

        DynamicLibrary(DynamicLibrary const&) = delete;
        auto operator=(DynamicLibrary const&) -> DynamicLibrary& = delete;

        DynamicLibrary(DynamicLibrary&& other) noexcept : obj(other.obj) {
            other.obj = nullptr;
        }

        auto operator=(DynamicLibrary&& other) noexcept -> DynamicLibrary& {
            if (this != &other) {
                if (obj) SDL_UnloadObject(obj);
                obj = other.obj;
                other.obj = nullptr;
            }
            return *this;
        }

        ~DynamicLibrary() noexcept {
            if (obj) {
                SDL_UnloadObject(obj);
                obj = nullptr;
            }
        }

        auto sym(char const* name) const -> void* {
            auto ret = SDL_LoadFunction(obj, name);
            if (not ret) throw std::runtime_error(SDL_GetError());
            return (void*) ret;
        }
    };
}
