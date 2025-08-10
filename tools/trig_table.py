# A script to generate trigonometric tables for the fixed point arithmetic used by the game.
import math

N = 360   # table size
Q = 8     # fractional bits
SCALE = 1 << Q
MASK16 = (1 << 16) - 1

def format_fixed_display(u16):
    """u16 = stored 16-bit unsigned value.
    Print as signed fixed-point with fractional byte 0..255.
    Format: [+/-]INT.FFF  (FFF is 0..255, the raw fractional bits)
    """
    u16 &= MASK16
    # convert to signed 16-bit
    if u16 & 0x8000:
        sval = u16 - (1 << 16)
    else:
        sval = u16
    # integer part using truncation toward zero (C-style)
    whole = int(sval / SCALE)      # trunc toward zero
    frac = abs(sval) & (SCALE - 1) # fractional bits 0..255
    sign = '-' if sval < 0 else ''
    return f"{sign}{abs(whole)}.{frac:03d}"

print("sine table:")
for i in range(N):
    f = math.sin(math.radians(i))
    fixed = int(round(f * SCALE)) & MASK16   # stored as 16-bit two's complement
    print(format_fixed_display(fixed) + ",")

print("\ncosine table:")
for i in range(N):
    f = math.cos(math.radians(i))
    fixed = int(round(f * SCALE)) & MASK16
    print(format_fixed_display(fixed) + ",")
