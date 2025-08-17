# Sonic

An implementation of a generic, extensible Sonic engine designed to be small in implementation and fun to use.

## About the project

Why this game:
- Interesting physics more involved than most tile based games.
- I always wanted to implement a sonic game.
- I think the sonic fan game space would benefit from an open and portable engine.
- I had no other 2d game ideas.

## My version

The minimum I will definitely make:
- The first and most iconic Sonic level, Act 1 of Green Hill Zone from the first game.
- Make sure the code can compile in C++17 mode.
- Hot reloading of game object classes at runtime (really cool).
- Try to ensure it compiles in sad environments like Visual Studio.

If I have the time I will also make:
- Act 2 and 3 since they share most of their assets with the first stage. (Spoke too soon, the physics were hard to implement)

## Getting started

The project can be built using Visual Studio or the terminal. The dependencies are:

- C++ standard library
- SDL3

Simply open the project **directory** in Visual Studio. You might have to wait a moment for it to configure the project.
There are Visual Studio specific files but no solution file. It works and I don't have to configure
anything twice this way, which is pretty important with a more advanced runtime model.

Note that there are many targets because objects are built as hot-reloadable libraries.
Use `Build > Build All` (Ctrl+Shift+B) to build all the targets at once.
If you don't do that the engine will not find the objects when deserializing the level and throw.

The way I configured the Visual Studio project it might ask to install additional components if they
were skipped during the initial Visual Studio installation process. Nowadays Visual Studio has the convenient
option in the menu to use a more robust compiler, which doesn't change much except you don't need stars to align
for it to reliably compile code... :(

Besides the hereby documented Visual Studio jank it otherwise works fine. I did my best to get it working
more simply

## How to play

The controls can be operated in left handed and right handed modes:
- Move with arrow keys and jump with X
- Move with WASD keys and jump with ENTER
- Hold down to crouch while standing still or roll up while moving.
- Hold down and the jump button while still to charge up a spindash.
- (Misc) + and - keys adjust the integer scale the game is rendering at, the default is 3x.
- (Debug) Press 1 to toggle the visual debug overlay visualizing collision and more.
- (Debug) Press 2 to override physics and freely fly around.
- (Debug) Press 3 to toggle the object hitbox overlay (requires also enabling the general debug overlay).
- (Debug) Press 8 to toggle the heuristic refresh rate lock.
- (Debug) Press 9 to toggle the performance and refresh rate heuristic overlay.
- (Debug) Press 0 to toggle vsync.
- (Windows) Press F1 to toggle fullscreen since afaik the OS doesn't handle that at user-level.
- (Development) Press R to hot reload object classes (Windows only, on UNIX a reload will automatically signal the binary)

There were no other abilities in Sonic 1 yet.

---

## Project structure

### Directories

- include — All the engine headers.
- src — All the engine source code.
- object — The object plugin implementations, source code and headers.
- tools — Python scripts used to generate the sine/cosine tables etc.
- windows — Windows cross compilation toolchain and libraries.
- map — My binary file format extension to Tiled and the Tiled map project files.
- res — Resources used by the game.
- ref — References for making resources such as unedited sprite sheets found online.

### Build setup

- CMakeLists.txt — Build specification.
- CMakeSettings.json — Makes Visual Studio set up dependencies correctly on Windows.
- file_format_spec.txt — The specification of the binary file formats used.
- makefile — Conveniences for performing various tasks on the project.

---

## Class structure

The game itself uses a very standard class structure.
Do note that the game objects are in the object directory, not src as they are compiled as libraries
and loaded by the engine proper at runtime as needed.

```
Game -> Scene
          |-> Stage -> [(dynlib) Object -> [Trait]]
          |-> ... and potentially other simple scenes like the opening screen or death screen.
```

One could imagine a nesting here where a Scene composes another scene, wrapping and forwarding into it.
For example, in the original game when idle on the title screen the game enters a demo mode which
plays back pre-recorded gameplay for some levels.
Well, that can be done with a special scene which forwards transparently with the exception of Input,
constructing "fake" input from saved frame data instead of the engine provided actual input.

Because all the code is almost entirely pure and everything is forwarded through the hierarchy as in proper object oriented hierarchy design, this kind of composition will scale safely and easily.

So, scenes are designed to be nested in this graph. Really, the "Game" should probably be removed replaced
with better engine code for managing resources as that is its only function at the moment.

Notably the graph is not bi-directional, the parent or Io is passed as needed as parameters. This is
definitely subject to change at some point in order to focus working on game objects more on
the contract of being an object rather than explicitly passing context down, as that could be
handled by the `Object` supertype integrating with the `Stage`.

### Object composition

There is over 5000 actual lines of code in this project, lots of objects are composed and I can't really document them all here. The `plane.hpp` header has detailed documentation about the most notable example :)
It's a small namespace of a design I have implemented before in Swift, Rust and Kotlin, iterating over it
for many years now. I think it works decently well in C++ too, and with C++23 concepts it would actually
much more radable and sane. To get it working in C++17 I relied on type traits instead.

It performs composition of functor objects to make graphics code very expressive,
I will show a cool example of such a function here:

```cpp
[[clang::always_inline]] [[gnu::hot]] [[gnu::const]]
auto solid_at(i32 x, i32 y) const -> bool {
    auto const& tile = solid_tile(x / 16, y / 16);
    return height_tiles
        | draw::grid(16, 16)
        | draw::tile(tile.x, tile.y)
        | draw::apply_if(tile.mirror_x, draw::mirror_x())
        | draw::apply_if(tile.mirror_y, draw::mirror_y())
        | draw::get(x % 16, y % 16)
        | draw::eq(draw::color::WHITE);
}
```

I made a std::ranges inspired `|` forwarding pipe operator which makes deeply nesting chains sequential and easy to read. I put extra effort into it to make this work in C++17 and it's honestly quite simple,
here is an example of the grid constructed as a temporary in the above chain:

```cpp
template <typename T> struct Grid final {
    static_assert(Plane<T>::value);

    T inner;
    i32 item_width, item_height;
  public:
    constexpr explicit Grid(T inner, i32 item_width, i32 item_height) noexcept
        : inner(inner), item_width(item_width), item_height(item_height) {}

    constexpr auto tile(i32 x, i32 y) const noexcept -> Slice<T> {
        return Slice(inner, x * item_width, y * item_height, item_width, item_height);
    }
};
```

Very simple, it just slices graphics into tiles! With optimizations all of this is optimized away
and performs exceptionally well which is cool.

The compositions can nest forever so you could have a `Grid<Slice<Ref<const Image>>>`. Very cool.

The composition happens by value which is important, and a SFINAE reference wrapper `Ref<T>` is provided
which is used to express references in these compositions.

Why is this important? consider the above example of `apply_if`, it applies another adapter
within itself, in cases like that you'd reference a temporary and you get instant UB. Fun.
Conversion to `Ref<T> or Ref<const T>` is implicit for any `T` like normal references but there is also a
`draw::as_ref()` adapter provided for edge cases.

Also notably, while this might *seem* dangerous it's actually completely fault tolerant, reading out of bounds
is legal in this system and has to be defined for all primitives, which makes compositions that
freely shift slices of infinite planes just work. For example this is the camera implementation:

```cpp
// Rendering into this will draw applying the camera offset automatically.
auto camera_target = target | draw::shift(camera_x, camera_y);
```

This just wraps the target in a mutable slice which forwards reads and writes at an offset.
Sure, it technically expresses an area misaligned with the storage but out of bounds writes are simply
ignored. That's just the semantics of the `Image` primitive though, in one of the implementations
I made a cool `InfiniteImage` which actually stores pixel reads and writes across an infinite plane storing it
in chunks, I used to implement a map in a minecraft mod.

### Inheritance

There are three main (both flat) inheritance hierarchies in the gameplay implementation itself:
- Scene, something currently being rendered and updated by the game.
- Object, a supertype to all dynamic (non-tile) game objects.
- Trait, a supertype to all traits which combine inheritance and composition of object code.

There's some other minor uses of inheritance mainly for metaprogramming purposes. Most of my abstraction
is achieved through C++ traits rather than inheritance.

Objects are quite notably very virtual because they are compiled into separate dynamic libraries
making them hot-swappable at runtime. Since objects see each other and are recompiled together
this means even non-virtual changes to the types are sound and propagate after reloading.

In order to avoid multiple and virtual inheritance and all the associated non-standard ABI nonsense I decided to
keep the hierarchy flat. That by itself would be pretty bad, what about player-enemy interactions for example?
do we check every enemy kind? C++ pushes one towards using inheritance here. We could have a layer in-between, something
like `HostileObject` and have enemies derive that instead. Like I said however, it's common knowledge nowadays that
this is difficult to scale.

The solution to this is the other flat hierarchy, `Trait`. They are mini object components which can be freely added or
removed at runtime. For example:

```cpp
/// An object which takes damage from the player on collision.
class TakesDamageFromPlayer final : public Object::Trait {
  public:
    using DamageHandler = auto (Object::*) () -> void;

    DamageHandler handler;

    template <typename T> TakesDamageFromPlayer(T handler) : handler(static_cast<DamageHandler>(handler)) {}

    void damage() {
        (object->*handler)();
    }
};
```

The association is made at runtime like so:

```cpp
class Chopper final : public Object, public DefaultCodable<Chopper> {
  public:
    Chopper() {
        add<DamagesPlayer>(DamagesPlayer::UnprotectedOnly);
        add<TakesDamageFromPlayer>(&Chopper::damage);
    }
```

and it fits right alongside normal inheritance-based runtime polymorphism:

```cpp
void collide_with(Object* other) noexcept override {
    if (const auto ring = dynamic_cast<Ring*>(other)) {
        ring->pick_up();
        rings += 1;
    }

    if (const auto damaging = other->trait<DamagesPlayer>()) {
        if ((not rolled_up or damaging->bypass_protection()) and invulnerability == 0) {
            damage_state = DamageState::Pending;
            damaged_by_position = other->position;
        } else if (const auto takes_damage = other->trait<TakesDamageFromPlayer>()) {
            takes_damage->damage();
        }
    }
}
```

Another use of inheritance is the loosely Zig inspired `Io` interface which neatly encodes function purity
into the function signature itself. All side effects are encapsulated within. I think that part of
their new design is really cool.

---

## Contact

TODO

## Acknowledgements

- [The Sonic Physics Guide](https://info.sonicretro.org/Sonic_Physics_Guide)
