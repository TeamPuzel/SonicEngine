#pragma once
#include <core>
#include <array>

namespace sonic {
    class Tile {
      public:
        std::array<u8, 16> const* height_array;
    };
}
