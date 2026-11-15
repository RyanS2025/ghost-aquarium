# 0005. Sub-pixel float motion with steering behaviors and a fixed timestep

- **Status:** Accepted. Amended by [ADR 0009](0009-motion-model.md): rigid bodies, flip-turns, coming and going.
- **Date:** 2026-11-14

## Context

Most ASCII aquariums move fish one character cell at a time, which is 6–12 pixels per jump. That looks choppy, which is the opposite of "fluid." Fish that move in straight lines and bounce off walls also look mechanical. Real fish ease into turns, school loosely and meander.

Frame times on a microcontroller vary (SPI pushes, and later WiFi). If the physics advanced by "one frame" each loop, the fish would speed up and slow down along with the frame rate.

## Decision

1. **Float positions and velocities in pixels.** Glyphs are drawn at the rounded pixel position, not at a text-grid cell.
2. **Steering behaviors** after Craig Reynolds ("Boids", 1987, and "Steering Behaviors for Autonomous Characters", 1999). Separation, alignment, cohesion, wander and edge avoidance are weighted forces, summed, capped at `maxForce`, and integrated into a velocity capped at the layer's `maxSpeed`. Fish only school with fish in their own depth layer.
3. **A fixed 60 Hz timestep** using an accumulator. Each loop adds the real elapsed time and runs as many 1/60 s steps as fit, **up to 5**. Any extra time is dropped.
4. **Glyph-facing hysteresis.** A fish flips its glyph only when `|vel.x|` is greater than `facingThreshold`.
5. **Seeded, deterministic randomness** (xorshift32) inside the simulation.

## Alternatives considered

| Option | Why not |
|---|---|
| Grid-based ASCII movement | Choppy. Fails the core "fluid" goal. |
| Variable timestep (`pos += vel * frameDt`) | Simple, but a long frame means a big step, so fish tunnel through walls and the steering can become unstable. Results also differ from run to run, so tests can't compare exact values. |
| Scripted paths or tweens | Smooth, but the fish don't react to each other. No schooling, so it doesn't feel alive. |
| Perlin noise fields | Lovely drift, but fish ignore each other. It could replace the wander term later. |

## Consequences

- Steering is O(n²) in the number of fish. At 14 fish that's 196 pair checks per step, which is trivial.
- Dropping time after a stall means the tank briefly runs slower than real time. That's acceptable for an ambient piece, and much better than teleporting fish.
- Determinism means a reported bug can be replayed exactly from its seed and step count.
