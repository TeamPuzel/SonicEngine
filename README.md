# Sonic

An implementation of Sonic CD.

## About the project

Why this game:
- Interesting physics.
- I had no other ideas.

## My version

The minimum I will definitely make:
- A full act of a Sonic CD stage.
- The entities.
- Port code back to C++17 (it's the same thing but type traits are very verbose so I'd rather use improved requires syntax during development)
- Conditionally port some things like inline assembly to work on MSVC.

If I have the time I will also make:
- All acts of a Sonic CD stage.
- The special stages between acts.
- The entire game of Sonic CD.

## Getting started

The project can be built on any platform using CMake or Visual Studio. The dependencies are:

- C++ standard library (only on Windows as msvc does not like not having a standard library)
- SDL2
- Legacy OpenGL

## How to play

TBD

## Class structure

TBD

### Object composition

TBD

### Inheritance

TBD

## Contact

TODO

## Acknowledgements

- [The misunderstood definition of Object Oriented Programming as intended by Alan Kay](https://youtu.be/QjJaFG63Hlo?si=7Vs-8_V1AsO8N8nx)
- [Mutable Value Semantics](https://www.youtube.com/watch?v=QthAU-t3PQ4)
- [Sonic Physics Guide](https://info.sonicretro.org/Sonic_Physics_Guide)
