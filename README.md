# Sonic

An implementation of Sonic 1.

## About the project

Why this game:
- Interesting physics more involved than most tile based games.
- I had no other ideas.

## My version

The minimum I will definitely make:
- The first and most iconic Sonic level, Act 1 of Green Hill Zone from the first game.
- Port code to compile in C++17 mode.
- Ensure it compiles with broken compilers like MSVC which can *crash* while parsing labels. Pathetic. It also runs out of memory
without horrible hacks. I would never rely on such low quality and proprietary software in a serious project.

If I have the time I will also make:
- Act 2 and 3 since they share most of their assets with the first stage.

## Getting started

The project can be built on any platform using CMake or Visual Studio. The dependencies are:

- C++ standard library
- SDL3

## How to play

- Move with arrow keys
- Jump with X
- Press down arrow to crouch when standing still or roll up when moving.

There were no other abilities in Sonic 1 yet.

Note that in Sonic 1 there is a speed cap unless rolled up so that would be the fastest way downhill.

## Class structure

TBD

### Object composition

TBD

### Inheritance

TBD

## Contact

TODO

## Acknowledgements

- [The definition of Object Oriented Programming as intended by Alan Kay](https://youtu.be/QjJaFG63Hlo?si=7Vs-8_V1AsO8N8nx)
- [Mutable Value Semantics](https://www.youtube.com/watch?v=QthAU-t3PQ4)
- [Sonic Physics Guide](https://info.sonicretro.org/Sonic_Physics_Guide)
