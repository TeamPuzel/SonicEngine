# A script to generate trigonometric tables for the fixed point arithmetic used by the game.
import math

N = 360  # Table size
Q = 8    # Fractional bits for 24b.8b
scale = 1 << Q

def format_fixed(val):
    whole = val >> Q
    frac = val & 0xFF
    return f"{whole}.{frac:03d}"

print("sine table:")
for i in range(N):
    angle = math.radians(i)
    f = math.sin(angle)
    fixed_val = int(round(f * scale))
    # Wrap into 0–255 range if needed (optional depending on sign)
    if fixed_val < 0:
        fixed_val = (1 << 16) + fixed_val  # Two's complement
    print(f"{format_fixed(fixed_val)},")

print("\ncosine table:")
for i in range(N):
    angle = math.radians(i)
    f = math.cos(angle)
    fixed_val = int(round(f * scale))
    if fixed_val < 0:
        fixed_val = (1 << 16) + fixed_val
    print(f"{format_fixed(fixed_val)},")
