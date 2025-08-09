// Created by Lua (TeamPuzel) on July 30th 2025.
// Copyright (c) 2025 All rights reserved.
//
// File loading procedures.
#pragma once
#include <primitive>
#include <vector>
#include <fstream>

namespace rt {
    /// Loads the entirety of a file into program memory.
    /// Prioritizes simplicity since there is no need to interface with the OS.
    static auto load(char const* filename) -> std::vector<u8> {
        // Open the file in binary mode and move the cursor to the end.
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (not file.is_open())
            throw std::runtime_error("Could not open file");

        // Get the file size.
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        // Allocate a buffer.
        std::vector<u8> buffer;
        buffer.resize(size);

        // Read the file data into the buffer.
        if (not file.read(reinterpret_cast<char*>(buffer.data()), size))
            throw std::runtime_error("Could not read file");

        file.close();
        return buffer;
    }

    /// A sound implementation of a binary data reader for serialization purposes.
    ///
    /// It reads data out by individual bytes so alignment is not relevant,
    /// among other workarounds to avoid undefined behavior.
    class BinaryReader final {
        std::vector<u8> const& data;
        usize cursor { 0 };

        BinaryReader(std::vector<u8> const& data [[clang::lifetimebound]]) : data(data) {}

      public:
        /// Returns a new, default reader for the given data.
        static auto of(std::vector<u8> const& data [[clang::lifetimebound]]) -> BinaryReader {
            return BinaryReader(data);
        }

        template <typename R> auto read() noexcept(noexcept(R::read(*this))) -> R {
            return R::read(*this);
        }

        auto u8() -> ::u8 {
            const auto ret = this->data.at(this->cursor);
            this->cursor += 1;
            return ret;
        }

        auto u16() -> ::u16 {
            const auto ret = endian::from_le_bytes<::u16>(
                this->data.at(this->cursor),
                this->data.at(this->cursor + 1)
            );
            this->cursor += 2;
            return ret;
        }

        auto u32() -> ::u32 {
            const auto ret = endian::from_le_bytes<::u32>(
                this->data.at(this->cursor),
                this->data.at(this->cursor + 1),
                this->data.at(this->cursor + 2),
                this->data.at(this->cursor + 3)
            );
            this->cursor += 4;
            return ret;
        }

        auto u64() -> ::u64 {
            const auto ret = endian::from_le_bytes<::u64>(
                this->data.at(this->cursor),
                this->data.at(this->cursor + 1),
                this->data.at(this->cursor + 2),
                this->data.at(this->cursor + 3),
                this->data.at(this->cursor + 4),
                this->data.at(this->cursor + 5),
                this->data.at(this->cursor + 6),
                this->data.at(this->cursor + 7)
            );
            this->cursor += 8;
            return ret;
        }

        auto i8() -> ::i8 {
            return *reinterpret_cast<::i8 const*>(&this->data.at(this->cursor));
        }

        auto i16() -> ::i16 {
            const auto ret = endian::from_le_bytes<::i16>(
                this->data.at(this->cursor),
                this->data.at(this->cursor + 1)
            );
            this->cursor += 2;
            return ret;
        }

        auto i32() -> ::i32 {
            const auto ret = endian::from_le_bytes<::i32>(
                this->data.at(this->cursor),
                this->data.at(this->cursor + 1),
                this->data.at(this->cursor + 2),
                this->data.at(this->cursor + 3)
            );
            this->cursor += 4;
            return ret;
        }

        auto i64() -> ::i64 {
            const auto ret = endian::from_le_bytes<::i64>(
                this->data.at(this->cursor),
                this->data.at(this->cursor + 1),
                this->data.at(this->cursor + 2),
                this->data.at(this->cursor + 3),
                this->data.at(this->cursor + 4),
                this->data.at(this->cursor + 5),
                this->data.at(this->cursor + 6),
                this->data.at(this->cursor + 7)
            );
            this->cursor += 8;
            return ret;
        }

        auto boolean() -> bool {
            const bool ret = this->data.at(this->cursor) ? true : false;
            this->cursor += 1;
            return ret;
        }

        /// Assume the current position to be a C string in a fixed buffer.
        /// Returns the C string and skips over the buffer.
        auto cstr(usize bufsize) -> char const* {
            // This cast is sound: u8 -> char (on arm architectures char is even already unsigned by default)
            // The alignment is a match.
            auto ret = reinterpret_cast<char const*>(&this->data.at(this->cursor));
            this->cursor += bufsize;
            return ret;
        }

        /// Returns the amount of data contained.
        auto size() const noexcept -> usize {
            return this->data.size();
        }

        /// Returns the current cursor offset.
        auto position() const noexcept -> usize {
            return this->cursor;
        }

        /// Resets the reader state to the start.
        void rewind() noexcept {
            this->cursor = 0;
        }

        /// Seeks to the specified byte position.
        void seek(usize position) noexcept {
            this->cursor = position;
        }

        /// Skips over the given number of bytes.
        void skip(usize count) noexcept {
            this->cursor += count;
        }
    };
}
