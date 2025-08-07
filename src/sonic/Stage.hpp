// Created by Lua (TeamPuzel) on May 26th 2025.
// Copyright (c) 2025 All rights reserved.
//
// This is where the level magic happens.
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
        draw::DrawableSlice<draw::Ref<const draw::Image>> height_arrays;
        u32 width { 0 };
        u32 height { 0 };
        std::vector<Tile> foreground;
        std::vector<SolidTile> collision;
        std::vector<box<Object>> objects;
        Object* primary;
        usize tick;

      public:
        bool visual_debug { false };
        bool movement_debug { false };

        Stage(draw::DrawableSlice<draw::Ref<const draw::Image>> height_arrays) : height_arrays(height_arrays) {}

        void update(rt::Input const& input) override {
            if (input.key_pressed(rt::Key::Num1)) visual_debug = !visual_debug;
            if (input.key_pressed(rt::Key::Num2)) movement_debug = !movement_debug;

            for (box<Object>& object : objects) {
                object->update(input, *this);
            }

            tick += 1;
        }

        /// We receive three things, an inout mutable image representing the screen to render into,
        /// another image which is the sprite sheet and one to slice the background from.
        void draw(draw::Ref<draw::Image> target, draw::Ref<const draw::Image> sheet, draw::Ref<const draw::Image> background) const override {
            // We will first assemble a buffer of draw commands, this way we can easily sort before rendering later.
            std::vector<DrawCommand> commands;

            const auto [_px, _py] = primary->tile_pos();
            const auto [_ppx, _ppy] = primary->pixel_pos();

            // Because we are using a terrible old C++ version we can't capture structured bindings in lambdas.
            // That's fine, I'll reassign them, stupid language >:(
            const auto px = _px, py = _py, ppx = _ppx, ppy = _ppy;

            // Rendering into this will draw applying the camera offset automatically.
            auto camera_target = target
                | draw::shift(-ppx + target.width() / 2, -ppy + target.height() / 2);

            // Obtain all the visible tiles and schedule them for rendering.
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

            // Schedule objects for rendering as well.
            // TODO: Avoid scheduling off-screen objects.
            for (box<Object> const& object : objects) {
                auto command = DrawCommand { DrawCommand::Type::Object };
                command.object.ref = *object;
                commands.push_back(command);
            }

            // We are now ready to start drawing the stage.

            // First clear the entire screen with the water color, just in case the display is taller than the parallax bg.
            // This is hardcoded for 1-1 at the moment.
            target | draw::clear(draw::Color::rgba(0, 144, 252));

            // The background will be drawn by:
            // - Slicing out the parallax strips from the background in a repeating fashion.
            // - Slicing a screen-width slice out of the now infinite strip at the parallax x offset.
            // - Drawing it to the screen at the y offset of the particular strip.
            //
            // Remember, the drawable system is fault tolerant by its definition as it describes infinite planes of pixels.
            // Even an Image has "pixels" out of bounds, they are just not stored and always clear (transparent).
            // But we can take advantage of this in creative ways and for example repeat the image out of bounds
            // with a simple functor. Modern compilers do an excellent job of inlining this away and optimizing it out!
            //
            // That's not all however. the final water strip is mapped over its color and position
            // in order to parallax shift individual lines. The original game did this across individual scanlines
            // simulating the water having z depth as it gets further away in great detail.
            // But there's more! the water colors themselves are mapped in a cycle
            // as that's how the original game creates the effect of light shimmering across the surface.
            //
            // This was all trivial tricks for the original hardware, changing palettes and scrolling between scanlines,
            // but it requires either shaders or software processing to make sense in a modern context like this.
            //
            // The following code snippet showcases beautifully how powerful this composable rendering abstaction is,
            // in just a few lines of code this effect is replicated without any existing stateful implementation.
            // Everything is just made of composable building blocks which can describe anything through intuitive expressions.
            {
                // Rotate through these colors for the waterfalls and shimmer.
                const draw::Color shimmer_colors[4] = {
                    draw::Color::rgba(108, 144, 180),
                    draw::Color::rgba(108, 144, 252),
                    draw::Color::rgba(144, 180, 252),
                    draw::Color::rgba(180, 216, 252),
                };

                const auto shimmer_effect = [this, shimmer_colors] (draw::Color color, i32 x, i32 y) -> draw::Color {
                    const i32 shift = i32(tick / 4) % 4;

                    if (color == draw::Color::rgba(119, 17, 119)) {
                        return shimmer_colors[(3 + shift) % 4];
                    } else if (color == draw::Color::rgba(153, 51, 153)) {
                        return shimmer_colors[(2 + shift) % 4];
                    } else if (color == draw::Color::rgba(187, 85, 187)) {
                        return shimmer_colors[(1 + shift) % 4];
                    } else if (color == draw::Color::rgba(221, 119, 221)) {
                        return shimmer_colors[(0 + shift) % 4];
                    } else {
                        return color;
                    }
                };

                // Tile the background infinitely. Wouldn't want to run out :)
                auto back = background | draw::repeat();

                target | draw::draw(back | draw::slice(ppx / 32, 0, target.width(), 16 * 2), 0, 0);
                target | draw::draw(back | draw::slice(ppx / 32, 16 * 2, target.width(), 16 * 1), 0, 16 * 2);
                target | draw::draw(back | draw::slice(ppx / 32, 16 * 3, target.width(), 16 * 1), 0, 16 * 3);
                target | draw::draw(back | draw::slice(ppx / 32, 16 * 4, target.width(), 16 * 3), 0, 16 * 4);
                target | draw::draw(
                    back
                        | draw::slice(ppx / 24, 16 * 7, target.width(), 16 * 2 + 8)
                        | draw::map(shimmer_effect),
                    0, 16 * 7
                );
                target | draw::draw(
                    back
                        | draw::slice(ppx / 24, 16 * 9 + 8, target.width(), 16 * 6 + 8)
                        | draw::map_pos([ppx] (i32 x, i32 y) { return math::point { x + y * ppx / (16 * 32), y }; })
                        | draw::map(shimmer_effect),
                    0, 16 * 9 + 8
                );
            }

            // We can now move on to drawing the sorted tiles and objects back to front.
            for (const auto command : commands) {
                if (command.type == DrawCommand::Type::Tile) {
                    auto const& tile = foreground.at(command.tile.y + command.tile.x * height);

                    auto tilemap = sheet
                        | draw::grid(16, 16);

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
                    const auto [x, y, w, h, mirror_x, mirror_y, rotate] = object.sprite();
                    const auto ofx = -w / 2;
                    const auto ofy = -h / 2;

                    auto tilemap = sheet
                        | draw::grid(w, h);
                    camera_target | draw::draw(
                        tilemap.tile(x, y)
                            | draw::apply_if(mirror_x, draw::mirror_x())
                            | draw::apply_if(mirror_y, draw::mirror_y()),
                        posx + ofx,
                        posy + ofy
                    );
                }
            }

            // If the debug visuals are enabled draw them and request objects do to so.
            if (visual_debug) {
                for (box<Object> const& object : objects) {
                    object->debug_draw(camera_target);
                }
            }
        }

        enum class SensorDirection : u8 { Up, Down, Left, Right };

        // TODO: Sensor implementation.
        /// The sensor logic is implemented differently, given the significant CPU improvement since then
        /// the game can just dynamically figure out the height arrays from the reference images (also used for debugging).
        auto sense() const -> auto {

        }

        /// Loads a stage from a file using the provided object registry.
        /// Throws a runtime error if the registry doesn't recognize the object.
        template <typename Reg> static auto load(
            char const* filename,
            Reg const& registry,
            draw::DrawableSlice<draw::Ref<const draw::Image>> height_arrays
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
