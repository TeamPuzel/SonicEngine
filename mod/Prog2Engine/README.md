# Prog2Engine

### Removal note

After more consideration not much about this library is a good fit for this game so it is no longer being used.
- Delta time is unsuitable for fixed point arithmetic recreating a 16 bit game documented assuming 60hz.
- Floating point numbers are unsuitable for rendering pixel graphics accurately. Most of the game can be
rendered simply by copying memory around and shaders are convenient to better mimic the appearance of a crt.