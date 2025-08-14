// Created by Lua (TeamPuzel) on May 26th 2025.
// Copyright (c) 2025 All rights reserved.
//
// This header defines everything about levels.
#pragma once
#include <primitive>
#include <rt>
#include <sstream>
#include <vector>
#include <functional>
#include "scene.hpp"
#include "object.hpp"
#include "class_loader.hpp"

namespace sonic {
    struct DrawCommand {
        enum class Type : u8 { Tile, Object } type;

        struct Tile final { i32 x, y; };
        struct Object final { std::reference_wrapper<const sonic::Object> ref; };

        union { Tile tile; Object object; };
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
        angle angle;
        Solidity solidity;
        bool flag;
        bool mirror_x;
        bool mirror_y;

        constexpr auto empty() const noexcept -> bool {
            return x == -1 and y == -1;
        }

        static auto read(rt::BinaryReader& reader) -> SolidTile {
            const auto x = reader.i32();
            const auto y = reader.i32();
            const auto raw_angle = reader.u16();
            const auto solidity = (Solidity) reader.u8();
            const auto mirror_x = reader.boolean();
            const auto mirror_y = reader.boolean();

            return SolidTile {
                x, y,
                math::angle(raw_angle),
                solidity,
                raw_angle == 360,
                mirror_x,
                mirror_y
            };
        }
    };

    /// A coroutine class representing the state of a loaded stage.
    class Stage final : public Scene {
        Ref<const Image> height_tiles;
        u32 width { 0 };
        u32 height { 0 };
        std::vector<Tile> foreground;
        std::vector<SolidTile> collision;
        std::vector<Box<Object>> objects;
        Object* primary;
        usize tick;

      public:
        bool visual_debug { false };
        bool movement_debug { false };

        [[gnu::hot]] [[gnu::const]]
        auto tile(i32 x, i32 y) const -> Tile const& {
            return foreground.at(y + x * height);
        }

        [[gnu::hot]] [[gnu::const]]
        auto solid_tile(i32 x, i32 y) const -> SolidTile const& {
            return collision.at(y + x * height);
        }

        Stage(Ref<const Image> height_tiles) : height_tiles(height_tiles) {}

        void update(Io& io, rt::Input const& input) override {
            if (input.key_pressed(rt::Key::Num1)) visual_debug = !visual_debug;
            if (input.key_pressed(rt::Key::Num2)) movement_debug = !movement_debug;

            // On UNIX this is automatic and mapped to SIGUSR1, no need for a keybind.
            // if (input.key_pressed(rt::Key::R)) hot_reload();

            const auto [px, py] = primary->pixel_pos();
            constexpr i32 X_UPDATE_DISTANCE = 320 + 320 / 2;
            constexpr i32 Y_UPDATE_DISTANCE = 224 + 224 / 2;

            // We don't update objects too far away from the primary.
            //
            // The original resolution is 320x224 so the approximation used will be only processing
            // objects when they are an original screen and a half distance away.
            for (Box<Object>& object : objects) {
                const auto [ox, oy] = object->pixel_pos();
                if (std::abs(ox - px) < X_UPDATE_DISTANCE and std::abs(oy - py) < Y_UPDATE_DISTANCE)
                    object->update(input, *this);
            }

            tick += 1;
        }

        /// We receive three things, an inout mutable image representing the screen to render into,
        /// another image which is the sprite sheet and one to slice the background from.
        [[gnu::hot]] void draw(
            Io& io, rt::Input const& input, Ref<Image> target, Ref<const Image> sheet, Ref<const Image> background
        ) const override {
            // We will first assemble a buffer of draw commands, this way we can easily sort before rendering later.
            std::vector<DrawCommand> commands;

            const auto [_px, _py] = primary->tile_pos();
            const auto [_ppx, _ppy] = primary->pixel_pos();

            // Because we are using a terrible old C++ version we can't capture structured bindings in lambdas.
            // That's fine, I'll reassign them, stupid language >:(
            const auto px = _px, py = _py, ppx = _ppx, ppy = _ppy;

            const i32 camera_x = std::min(-ppx + target.width() / 2, 0);
            const i32 camera_y = std::max(-ppy + target.height() / 2, -63 * 16 + target.height());

            const auto ccx = -camera_x + target.width() / 2;
            const auto ccy = -camera_y + target.height() / 2;

            // Rendering into this will draw applying the camera offset automatically.
            auto camera_target = target
                | draw::shift(camera_x, camera_y);

            // Obtain all the visible tiles and schedule them for rendering.
            {
                const auto tile_width = 16;
                const auto tile_height = 16;

                const auto half_screen_tiles_x = (i32(target.width()) / tile_width + 2) / 2; // +2 for safety margin
                const auto half_screen_tiles_y = (i32(target.height()) / tile_height + 2) / 2;

                const auto min_x = std::max(ccx / 16 - half_screen_tiles_x, 0);
                const auto max_x = std::min(ccx / 16 + half_screen_tiles_x + 1, i32(width));
                const auto min_y = std::max(ccy / 16 - half_screen_tiles_y, 0);
                const auto max_y = std::min(ccy / 16 + half_screen_tiles_y + 1, i32(height));

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
            // Objects more than a screen away from the edge are not drawn.
            {
                const i32 buffer_x = target.width();
                const i32 buffer_y = target.height();

                // Visible rectangle in world coordinates.
                const i32 view_min_x = -camera_x - buffer_x;
                const i32 view_max_x = -camera_x + target.width() + buffer_x;
                const i32 view_min_y = -camera_y - buffer_y;
                const i32 view_max_y = -camera_y + target.height() + buffer_y;

                for (Box<Object> const& object : objects) {
                    const auto [ox, oy] = object->pixel_pos();

                    if (ox >= view_min_x and ox <= view_max_x and
                        oy >= view_min_y and oy <= view_max_y
                    ) {
                        auto command = DrawCommand { DrawCommand::Type::Object };
                        command.object.ref = *object;
                        commands.push_back(command);
                    }
                }
            }

            // We are now ready to start drawing the stage.

            // First clear the entire screen with the water color, just in case the display is taller than the parallax bg.
            // This is hardcoded for 1-1 at the moment.
            target | draw::clear(Color::rgba(0, 144, 252));

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
                const Color shimmer_colors[4] = {
                    Color::rgba(108, 144, 180),
                    Color::rgba(108, 144, 252),
                    Color::rgba(144, 180, 252),
                    Color::rgba(180, 216, 252),
                };

                const auto shimmer_effect = [this, shimmer_colors] (Color color, i32 x, i32 y) -> Color {
                    const i32 shift = i32(tick / 4) % 4;

                    if (color == Color::rgba(119, 17, 119)) {
                        return shimmer_colors[(3 + shift) % 4];
                    } else if (color == Color::rgba(153, 51, 153)) {
                        return shimmer_colors[(2 + shift) % 4];
                    } else if (color == Color::rgba(187, 85, 187)) {
                        return shimmer_colors[(1 + shift) % 4];
                    } else if (color == Color::rgba(221, 119, 221)) {
                        return shimmer_colors[(0 + shift) % 4];
                    } else {
                        return color;
                    }
                };

                // Tile the background infinitely. Wouldn't want to run out :)
                auto back = background | draw::repeat();

                target | draw::draw(back | draw::slice(ccx / 32, 0, target.width(), 16 * 2), 0, 0);
                target | draw::draw(back | draw::slice(ccx / 32, 16 * 2, target.width(), 16 * 1), 0, 16 * 2);
                target | draw::draw(back | draw::slice(ccx / 32, 16 * 3, target.width(), 16 * 1), 0, 16 * 3);
                target | draw::draw(back | draw::slice(ccx / 32, 16 * 4, target.width(), 16 * 3), 0, 16 * 4);
                target | draw::draw(
                    back
                        | draw::slice(ccx / 24, 16 * 7, target.width(), 16 * 2 + 8)
                        | draw::map(shimmer_effect),
                    0, 16 * 7
                );
                target | draw::draw(
                    back
                        | draw::slice(ccx / 24, 16 * 9 + 8, target.width(), 16 * 6 + 8)
                        | draw::map_pos([ccx] (i32 x, i32 y) { return math::point { x + y * ccx / (16 * 32), y }; })
                        | draw::map(shimmer_effect),
                    0, 16 * 9 + 8
                );
            }

            // We can now move on to drawing the sorted tiles and objects back to front.
            for (const auto command : commands) {
                if (command.type == DrawCommand::Type::Tile) {
                    auto const& tile = this->tile(command.tile.x, command.tile.y);

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
                    const auto [x, y, w, h, mirror_x, mirror_y, rotation] = object.sprite(input);
                    const auto ofx = -w / 2;
                    const auto ofy = -h / 2;

                    auto tilemap = sheet
                        | draw::grid(w, h);
                    camera_target | draw::draw(
                        tilemap.tile(x, y)
                            | draw::apply_if(mirror_x, draw::mirror_x())
                            | draw::apply_if(mirror_y, draw::mirror_y())
                            | draw::rotate(i32(rotation)),
                        posx + ofx,
                        posy + ofy
                    );
                }
            }

            // Request the primary to draw the hud.
            primary->hud_draw(io, target, *this);

            // If the debug visuals are enabled draw them as well.
            if (visual_debug) {
                std::stringstream out;

                for (const auto command : commands) {
                    if (command.type == DrawCommand::Type::Tile) {
                        auto const& tile = this->solid_tile(command.tile.x, command.tile.y);

                        auto tilemap = height_tiles
                            | draw::grid(16, 16);

                        camera_target | draw::draw(
                            tilemap.tile(tile.x, tile.y)
                                | draw::map([] (Color color, i32 x, i32 y) -> Color {
                                    return color.with_a(128); // Intentionally increase transparency level too to darken everything.
                                })
                                | draw::apply_if(tile.mirror_x, draw::mirror_x())
                                | draw::apply_if(tile.mirror_y, draw::mirror_y()),
                            command.tile.x * 16, command.tile.y * 16,
                            draw::blend::alpha
                        );

                        if (not tile.empty()) {
                            std::stringstream angle_out;
                            if (not tile.flag) angle_out << (u32) tile.angle; else angle_out << "flg";

                            camera_target | draw::draw(
                                Text(angle_out.str(), font::pico(io)),
                                command.tile.x * 16, command.tile.y * 16
                            );
                        }
                    }
                    if (command.type == DrawCommand::Type::Object) {
                        Object const& object = command.object.ref.get();
                        object.debug_draw(out, camera_target, *this);
                    }
                }

                std::string line;
                for (i32 y = 8; std::getline(out, line); y += font::mine(io).height + font::mine(io).leading) {
                    target | draw::draw(Text(line, font::mine(io)), 8, y);
                }
            }
        }

        enum class SensorDirection : u8 { Down, Right, Up, Left };

        struct SensorResult final { i32 distance; math::angle angle; bool flag; };

        /// Inlining this is rather important, we are doing some composition and this is called in a loop
        /// so we'd get janky stack shenanigans if we actually called this.
        [[clang::always_inline]] [[gnu::hot]] [[gnu::const]] auto solid_at(i32 x, i32 y) const -> bool {
            auto const& tile = solid_tile(x / 16, y / 16);
            return height_tiles
                | draw::grid(16, 16)
                | draw::tile(tile.x, tile.y)
                | draw::apply_if(tile.mirror_x, draw::mirror_x())
                | draw::apply_if(tile.mirror_y, draw::mirror_y())
                | draw::get(x % 16, y % 16)
                | draw::eq(draw::color::WHITE);
        }

        /// The sensor logic is implemented differently, given the significant CPU improvement since then.
        /// We just analyze the height map in pixel-space instead of the 1991 logic of regressing through tiles.
        /// At the end of the day objects just want the distance and they do not care if the entire range is consistent
        /// as they always considered only the consistent subrange within. I can't believe I spent days on
        /// this nonsense instead of just doing the obious thing.
        [[gnu::const]] auto sense(i32 x, i32 y, SensorDirection direction) const -> SensorResult {
            i32 cx = x, cy = y;

            const i32 max_distance = 32;

            const auto regress = [&] {
                switch (direction) {
                    case SensorDirection::Down:  cy -= 1; break;
                    case SensorDirection::Right: cx -= 1; break;
                    case SensorDirection::Up:    cy += 1; break;
                    case SensorDirection::Left:  cx += 1; break;
                }
            };

            const auto extend = [&] {
                switch (direction) {
                    case SensorDirection::Down:  cy += 1; break;
                    case SensorDirection::Right: cx += 1; break;
                    case SensorDirection::Up:    cy -= 1; break;
                    case SensorDirection::Left:  cx -= 1; break;
                }
            };

            const auto distance = [&] {
                switch (direction) {
                    case SensorDirection::Down:  return cy - y;
                    case SensorDirection::Right: return cx - x;
                    case SensorDirection::Up:    return y - cy;
                    case SensorDirection::Left:  return x - cx;
                }
            };

            const auto within_limit = [&] {
                const auto d = distance();
                return -32 <= d and d <= 32;
            };

            // There are two cases, either we are within terrain and we need to regress or we are not inside of terrain
            // and we need to extend.
            if (solid_at(cx, cy)) {
                do regress(); while (solid_at(cx, cy) and within_limit());
            } else {
                do extend(); while (not solid_at(cx, cy) and within_limit());
                regress();
            }

            auto const& tile = solid_tile(cx / 16, cy / 16);

            return { distance(), tile.angle, tile.flag };
        }

        [[clang::always_inline]] [[gnu::const]]
        auto sense(Object const* relative_space, i32 x, i32 y, SensorDirection direction) const -> SensorResult {
            auto [ox, oy] = relative_space->pixel_pos();
            return sense(x + ox, y + oy, direction);
        }

        [[clang::always_inline]] [[gnu::const]]
        static auto rotate(SensorDirection direction, u32 by_steps) noexcept -> SensorDirection {
            return (SensorDirection) (((u32) direction + by_steps) % 4);
        }

        [[clang::always_inline]] [[gnu::const]]
        static auto rotate(i32 x, i32 y, i32 steps) noexcept -> std::pair<i32, i32> {
            steps = ((steps % 4) + 4) % 4;

            switch (steps) {
                case 0: return { +x, +y };
                case 1: return { +y, -x };
                case 2: return { -x, -y };
                case 3: return { -y, +x };
            }

            intr::unreachable();
        }

        [[clang::always_inline]] [[gnu::const]]
        auto sense(Object const* relative_space, i32 x, i32 y, SensorDirection direction, Object::Mode mode) const -> SensorResult {
            const auto [rx, ry] = rotate(x, y, (i32) mode);
            return sense(relative_space, rx, ry, rotate(direction, (u32) mode));
        }

        /// Visualises a sensor within a target.
        /// The target's origin should align with the relative space origin and need not have size.
        template <typename T>
        void sense_draw(Object const* relative_space, i32 x, i32 y, SensorDirection direction, T target, Color color) const {
            static_assert(draw::MutablePlane<T>::value);
            const auto res = sense(relative_space, x, y, direction);

            switch (direction) {
                case SensorDirection::Down:  target | draw::line(x, y, x, y + res.distance, color); break;
                case SensorDirection::Right: target | draw::line(x, y, x + res.distance, y, color); break;
                case SensorDirection::Up:    target | draw::line(x, y, x, y - res.distance, color); break;
                case SensorDirection::Left:  target | draw::line(x, y, x - res.distance, y, color); break;
            }
        }

        template <typename T>
        void sense_draw(Object const* relative_space, i32 x, i32 y, SensorDirection direction, Object::Mode mode, T target, Color color) const {
            static_assert(draw::MutablePlane<T>::value);
            const auto res = sense(relative_space, x, y, direction, mode);

            auto rotated_target = target
                | draw::rotate_global((u8) mode);

            switch (direction) {
                case SensorDirection::Down:  rotated_target | draw::line(x, y, x, y + res.distance, color); break;
                case SensorDirection::Right: rotated_target | draw::line(x, y, x + res.distance, y, color); break;
                case SensorDirection::Up:    rotated_target | draw::line(x, y, x, y - res.distance, color); break;
                case SensorDirection::Left:  rotated_target | draw::line(x, y, x - res.distance, y, color); break;
            }
        }

        /// Loads a stage from a file using a provided object registry.
        /// Throws a runtime error if the object class does not exist.
        static auto load(Io& io, std::string_view filename, Ref<const Image> height_arrays) -> Box<Stage> {
            auto ret = Box<Stage>::make(height_arrays);

            const auto data = io.read_file(filename);
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

                const auto descriptor = class_loader::load(io, classname);

                const auto x = reader.i32();
                const auto y = reader.i32();

                const auto position = reader.position();

                auto instance = descriptor.deserializer(reader);
                instance->position = math::point<fixed> { x, y };
                if (classname == "Sonic") ret->primary = instance.raw();
                instance->classname = classname;
                ret->objects.emplace_back(std::move(instance));

                reader.seek(position + 1024);
            }

            return ret;
        }

        [[gnu::cold]] void hot_reload(Io& io) override {
            class_loader::swap_registry();
            for (Box<Object>& object : objects) {
                if (not object->is_dynobject()) continue;

                auto descriptor = class_loader::load(io, object->classname);
                auto replacement = descriptor.rebuilder(*object);

                replacement->position = object->position;
                replacement->classname = object->classname;
                if (object->classname == "Sonic") primary = replacement.raw();

                std::swap(object, replacement);
            }
            class_loader::drop_old_object_classes();
        }

        ~Stage() noexcept {
            /// Make sure that we no longer hold on to objects, we can't destroy them after clearing the class loader.
            objects.clear();
            class_loader::clear();
        }
    };
}
