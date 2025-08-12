# Sonic

An implementation of Sonic 1.

## About the project

Why this game:
- Interesting physics more involved than most tile based games.
- I had no other ideas.

## My version

The minimum I will definitely make:
- The first and most iconic Sonic level, Act 1 of Green Hill Zone from the first game.
- Make sure the code can compile in C++17 mode.
- Hot reloading of game object classes at runtime (really cool).
- Try to ensure it compiles with sad compilers like MSVC.

If I have the time I will also make:
- Act 2 and 3 since they share most of their assets with the first stage. (Spoke too soon, the physics were hard to implement)

## Getting started

The project can be built on any platform using CMake or Visual Studio. The dependencies are:

- C++ standard library
- SDL3
- (ideally) gnu-make, cmake and ninja (visual studio only compiles the engine without hot-reloading)

## How to play

The controls can be operated in left handed and right handed modes:
- Move with arrow keys and jump with X
- Move with WASD keys and jump with ENTER
- Hold down to crouch while standing still or roll up while moving.
- Hold down and the jump button while still to charge up a spindash.
- (Misc) + and - keys adjust the integer scale the game is rendering at, the default is 4x.
- (Debug) Press 1 to toggle the visual debug overlay visualizing collision and more.
- (Debug) Press 2 to override physics and freely fly around.
- (Debug) Press 8 to toggle the heuristic refresh rate lock.
- (Debug) Press 9 to toggle the performance and refresh rate heuristic overlay.
- (Debug) Press 0 to toggle vsync.
- (Development) Press R to hot reload object classes.

There were no other abilities in Sonic 1 yet.

Note that in Sonic 1 there is a speed cap unless rolled up so that would be the fastest way downhill.

## Class structure

The game itself uses a very standard class structure.
Do note that the game objects are in the object directory, not src as they are compiled as libraries
and loaded by the engine proper at runtime as needed.

```
Game -> Scene
          |-> Stage -> [(dynlib) Object]
          |-> ... and potentially other simple scenes like the opening screen or death screen.
```

### Object composition

The top of the `Drawable.hpp` header has *very* comprehensive documentation of composition
using complex std::ranges inspired adapters which is used for generic composition of drawing operations.

That's probably the most notable example since I have a lot of code here and can't realistically describe everything.

### Inheritance

There are two main (both flat) inheritance hierarchies in the gameplay implementation itself:
- Scene, something currently being rendered and updated by the game.
- Object, a supertype to all dynamic (non-tile) game objects.

There's some other minor uses of inheritance mainly for metaprogramming purposes. Most of my abstraction
is achieved through C++ traits rather than inheritance.

Objects are quite notably very virtual because they are compiled into separate dynamic libraries
making them hot-swappable at runtime. Since objects see each other and are recompiled together
this means even non-virtual changes to the types are sound and propagate after reloading.

## Contact

TODO

## Acknowledgements

- [Sonic Physics Guide](https://info.sonicretro.org/Sonic_Physics_Guide)
