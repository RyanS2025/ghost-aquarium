# 0009. Motion model: rigid ASCII fish, flip-turns, coming and going

- **Status:** Accepted
- **Date:** 2026-11-14
- **Amends:** [ADR 0005](0005-motion-subpixel-steering.md). Float positions, steering forces and the fixed timestep all stand; this ADR replaces how bodies, turns and walls behave.

## Context

ADR 0005's steering model produced fish that *moved* fluidly but *looked* messy. Each iteration in the prototype was measured, not just eyeballed:

| Attempt | Problem measured |
|---|---|
| Characters following the swim path | One fish's body bent from −22° to +47°. ASCII is only readable as a straight row, so fish read as scattered punctuation |
| U-turns through arcs | A 56 px body around a 16–36 px turning circle curls into a vertical column (26% of samples) |
| Wander as a world-space direction | It pointed backward every few seconds, so fish kept reversing |
| Schooling allowed to reverse fish | Separation from the fish ahead pushes backward. 70% of reversals came from schooling, and schools flipped each other back and forth (66 flips/fish/min) |
| Side walls always on | Constant turning at the glass; walls were the main reason any fish turned |

## Decision

1. **Rigid bodies:** a straight, evenly spaced row of characters behind the head, with a smoothed visual tilt of at most 8° and a tail wiggle under 1 px.
2. **The flip-turn is the only way to reverse.** The body shrinks around its middle (as if turning toward you), vx reverses and the glyph swaps at the halfway point, then the body grows back. Course changes that would cross vertical become flip-turns. The heading turns at most 1.6 rad/s and stays within 25° of level.
3. **Reversals come from navigation only** (walls + wander), with a 0.25 s debounce and a 3 s cooldown (walls override the cooldown). Schooling only bends the course or changes speed.
4. **Reynolds wander:** the fish steers toward a point that drifts around a circle placed ahead of it, so wander can't point backward.
5. **Wall priority:** near a wall, other forces are scaled down and the force budget and turn rate go up, so fast fish can turn before leaving the screen.
6. **Coming and going:** fish swim out the sides and return 2–7 s later from a random side, at the open depth farthest from their layer-mates. A cap keeps the tank from emptying: exits are reserved, so a fish takes a slot (35% of the fish) when it enters the side margin heading out, and fish without a slot see the wall early enough to turn.
7. **Spacing:** separation is measured to the nearest point of each neighbor's *body* (not just its head), scaled by body size per layer. There is light separation across layers too. Preferred depths are spread evenly so fish use the whole tank.

## Consequences

Measured in the prototype over 15–30 simulated minutes:

- 0% glyph/heading mismatch, 0 vertical escapes, no NaN
- about 0.5 turns per fish per minute, about 10 of 12 fish on screen (never fewer than 7), never away longer than 7 s
- about 2% overlap between any two fish (including across layers)

Other effects:

- Simpler firmware: no per-fish path history (which would have been about 29 KB).
- The away cap is exact because exits are reserved in advance. Found while porting: switching side walls on for everyone when the cap filled let fish a few pixels from the edge drift out anyway (6 away against a cap of 4).
- These numbers become `tanksim` tests (soak, determinism, bounds, facing) so the C++ port must reproduce them.
