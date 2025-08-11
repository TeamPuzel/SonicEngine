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
- Ensure it compiles with broken compilers like MSVC which can *crash* while parsing labels. Pathetic. It also runs out of memory
without horrible hacks. I would never rely on such low quality and proprietary software in a serious project :)

If I have the time I will also make:
- Act 2 and 3 since they share most of their assets with the first stage. (Spoke too soon, the physics were hard to implement)

## Getting started

The project can be built on any platform using CMake or Visual Studio. The dependencies are:

- C++ standard library
- SDL3

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

There were no other abilities in Sonic 1 yet.

Note that in Sonic 1 there is a speed cap unless rolled up so that would be the fastest way downhill.

## Class structure

The game itself uses a very boring, standard class structure.

```
Game -> Scene
          |-> Stage -> [Object]
          |-> ... and many other simple scenes like the opening screen or death screen.
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

There is more advanced subtyping present with a deeper subtyping hierarchy but it is again using traits for efficiency.
Specifically, `Drawable` the infinite supertype of all graphics, `SizedDrawable`, subtype of `Drawable` describing a concrete
area of the conceptual infinite plane, `MutableDrawable` which is quite self explanatory and (an unwritten, used individually) intersection of
these two `Drawable` subtypes required for rendering, `SizedDrawable + MutableDrawable` or in other words `SizedMutableDrawable`.
This is beyond the capability of most inheritance systems and while C++ has multiple inheritance this is not expressible
well and on top of that INCREDIBLY inefficient. As soon as these traits would implement virtual dispatch C++ compilers
would never optimize and devirtualize them (I have tested it). Dynamic, uninlined dispatch through a dozen nested virtual
calls for every single pixel drawn is painfully slow, obviously, and is very viral in that these dynamic constructs can
only deal with other dynamic constructs themselves, propagating inefficiency all the way through.

## Contact

TODO

## Acknowledgements

- [Sonic Physics Guide](https://info.sonicretro.org/Sonic_Physics_Guide)
