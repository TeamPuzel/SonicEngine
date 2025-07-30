// Created by Lua (TeamPuzel) on July 30th 2025.
// Copyright (c) 2025 All rights reserved.
//
// File loading procedures.
#pragma once
#include <primitive>
#include <type_traits>
#include <vector>
#include <fstream>

namespace rt {
    /// Loads the entirety of a file into program memory.
    /// Prioritizes simplicity since there is no need to interface with the OS.
    ///
    /// Only the bare minimum error handling is performed, that is, no data is returned
    /// if anything goes wrong.
    auto load(char const* filename) -> std::vector<u8> {
        // Open the file in binary mode and move the cursor to the end
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (not file.is_open())
            throw std::runtime_error("Could not open file");

        // Get the file size
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        // Allocate a buffer to hold the file's contents
        std::vector<u8> buffer;
        buffer.resize(size);

        // Read the file data into the buffer
        if (not file.read(reinterpret_cast<char*>(buffer.data()), size))
            throw std::runtime_error("Could not read file");

        file.close();
        return buffer;
    }

    class BinaryReader final {
        std::vector<u8> const& data;
        usize cursor;

        BinaryReader(std::vector<u8> const& data [[clang::lifetimebound]]) : data(data) {}

      public:
        static auto of(std::vector<u8> const& data [[clang::lifetimebound]]) -> BinaryReader {
            return BinaryReader(data);
        }

        /// Assume the current position to be a trivial value.
        template <typename R> auto read() -> R requires std::is_trivial<R>::value {
            auto ret = reinterpret_cast<R const*>(&this->data.at(this->cursor));
            this->cursor += sizeof(R);
            return *ret;
        }

        template <typename R>
            requires std::is_trivial<R>::value
        class Iterator final {
            R const* begin_ptr;
            R const* end_ptr;

            friend class BinaryReader;

          public:
            auto begin() -> R const* {
                return this->begin_ptr;
            }

            auto end() -> R const* {
                return this->end_ptr;
            }
        };

        /// Assume the current position to be a sequence of trivial values.
        template <typename R> auto read(usize count) [[clang::lifetimebound]] -> Iterator<R>
            requires std::is_trivial<R>::value
        {
            auto begin = reinterpret_cast<R const*>(&this->data.at(this->cursor));
            auto end = begin + count;
            this->cursor += sizeof(R) * count;
            return { begin, end };
        }

        /// Assume the current position to be a C string in a fixed buffer.
        auto cstr(usize bufsize) -> char const* {
            auto ret = reinterpret_cast<char const*>(&this->data.at(this->cursor));
            this->cursor += bufsize;
            return ret;
        }

        auto size() const -> usize {
            return this->data.size();
        }

        void rewind() {
            this->cursor = 0;
        }

        void seek(usize position) {
            this->cursor = position;
        }

        void skip(usize count) {
            this->cursor += count;
        }
    };
}
