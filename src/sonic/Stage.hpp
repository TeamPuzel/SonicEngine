// Created by Lua (TeamPuzel) on May 26th 2025.
// Copyright (c) 2025 All rights reserved.
#pragma once
#include <primitive>
#include <rt>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <functional>
#include "Scene.hpp"
#include "Object.hpp"

namespace sonic {
    struct DrawCommand {
        enum class Type : u8 {
            Tile,
            Object,
        } type;

        struct Tile final {
            i32 x, y;
        };

        struct Object final {
            std::reference_wrapper<const sonic::Object> ref;
        };

        union {
            Tile tile;
            Object object;
        };
    };

    struct Tile final {
        i32 x, y;
        bool mirror_x;
        bool mirror_y;

        static auto read(rt::BinaryReader& reader) -> Tile {
            return Tile {
                reader.i32(),
                reader.i32(),
                reader.boolean(),
                reader.boolean(),
            };
        }
    };

    enum class Solidity : u8 {
        Full,
        Top,
        SidesAndBottom,
    };

    struct SolidTile final {
        i32 x, y;
        u16 angle;
        Solidity solidity;
        bool mirror_x;
        bool mirror_y;

        static auto read(rt::BinaryReader& reader) -> SolidTile {
            return SolidTile {
                reader.i32(),
                reader.i32(),
                reader.u16(),
                (Solidity) reader.u8(),
                reader.boolean(),
                reader.boolean(),
            };
        }
    };

    /// A coroutine class representing the state of a loaded stage.
    class Stage final : public Scene {
        draw::DrawableSlice<const draw::Image> height_arrays;
        u32 width { 0 };
        u32 height { 0 };
        std::vector<Tile> foreground;
        std::vector<SolidTile> collision;
        std::vector<box<Object>> objects;
        Object* primary;

      public:
        bool visual_debug { true };

        Stage(draw::DrawableSlice<const draw::Image> height_arrays) : height_arrays(height_arrays) {}

        void update(rt::Input const& input) override {
            if (input.key_pressed(rt::Key::Escape)) visual_debug = !visual_debug;

            for (box<Object>& object : objects) {
                object->update(input, *this);
            }
        }

        void draw(draw::Image& target, draw::Image const& sheet) const override {
            std::vector<DrawCommand> commands;

            const auto [px, py] = primary->tile_pos();
            const auto [ppx, ppy] = primary->pixel_pos();

            auto camera_target = target
                | draw::shift(-ppx + target.width() / 2, -ppy + target.height() / 2);

            {
                const auto tile_width = 16;
                const auto tile_height = 16;

                const auto half_screen_tiles_x = (i32(target.width()) / tile_width + 2) / 2; // +2 for safety margin
                const auto half_screen_tiles_y = (i32(target.height()) / tile_height + 2) / 2;

                const auto min_x = std::max(px - half_screen_tiles_x, 0);
                const auto max_x = std::min(px + half_screen_tiles_x + 1, i32(width));
                const auto min_y = std::max(py - half_screen_tiles_y, 0);
                const auto max_y = std::min(py + half_screen_tiles_y + 1, i32(height));

                for (i32 x = min_x; x < max_x; x += 1) {
                    for (i32 y = min_y; y < max_y; y += 1) {
                        auto command = DrawCommand { DrawCommand::Type::Tile };
                        command.tile.x = x;
                        command.tile.y = y;
                        commands.push_back(command);
                    }
                }
            }

            for (box<Object> const& object : objects) {
                auto command = DrawCommand { DrawCommand::Type::Object };
                command.object.ref = *object;
                commands.push_back(command);
            }

            for (const auto command : commands) {
                if (command.type == DrawCommand::Type::Tile) {
                    auto const& tile = foreground.at(command.tile.y + command.tile.x * height);
                    auto tilemap = sheet | draw::grid(16, 16);
                    camera_target | draw::draw(
                        tilemap.tile(tile.x, tile.y)
                            | draw::apply_if(tile.mirror_x, draw::mirror_x())
                            | draw::apply_if(tile.mirror_y, draw::mirror_y()),
                        command.tile.x * 16, command.tile.y * 16
                    );
                }
                if (command.type == DrawCommand::Type::Object) {
                    Object const& object = command.object.ref.get();
                    const auto [posx, posy] = object.pixel_pos();
                    const auto [x, y, w, h] = object.sprite();
                    const auto [ofx, ofy] = object.sprite_offset();
                    auto tilemap = sheet | draw::grid(w, h);
                    camera_target | draw::draw(tilemap.tile(x, y), posx + ofx, posy + ofy);
                }
            }

            if (visual_debug) {


                for (box<Object> const& object : objects) {
                    object->debug_draw(camera_target);
                }
            }
        }

        enum class SensorDirection : u8 { Up, Down, Left, Right };

        // TODO: Sensor implementation.
        auto sense() -> auto {

        }

        /// Loads a stage from a file using the provided object registry.
        /// Throws a runtime error if the registry doesn't recognize the object.
        template <typename Reg> static auto load(
            char const* filename,
            Reg const& registry,
            draw::DrawableSlice<const draw::Image> height_arrays
        ) -> box<Stage> {
            static_assert(ObjectRegistry<Reg>::value);

            auto ret = box<Stage>::make(height_arrays);

            const auto data = rt::load(filename);
            auto reader = rt::BinaryReader::of(data);

            ret->width = reader.u32();
            ret->height = reader.u32();

            ret->foreground.reserve(ret->width * ret->height);
            ret->collision.reserve(ret->width * ret->height);

            for (u32 i = 0; i < ret->width * ret->height; i += 1) {
                ret->foreground.push_back(reader.read<Tile>());
            }

            for (u32 i = 0; i < ret->width * ret->height; i += 1) {
                ret->collision.push_back(reader.read<SolidTile>());
            }

            const auto object_count = reader.u32();
            ret->objects.reserve(object_count);

            for (u32 i = 0; i < object_count; i += 1) {
                const std::string_view classname = reader.cstr(64);

                const auto deserializer = registry(classname);
                if (not deserializer) {
                    std::stringstream msg;
                    msg << "Attempted to deserialize a class not present in the provided registry: class ";
                    msg << classname;
                    throw std::runtime_error(msg.str());
                }

                const auto x = reader.i32();
                const auto y = reader.i32();

                const auto position = reader.position();

                auto instance = deserializer(reader);
                instance->position = math::point<fixed> { x, y };
                if (classname == "Sonic") ret->primary = instance.raw();
                ret->objects.emplace_back(std::move(instance));

                reader.seek(position + 1024);
            }

            return ret;
        }
    };

    static auto registry(std::string_view classname) -> Deserializer* {
        if (classname == "Ring")  return Ring::deserialize;
        if (classname == "Sonic") return Sonic::deserialize;
        return nullptr;
    }
}
