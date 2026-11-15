# 0001. Record architecture decisions

- **Status:** Accepted
- **Date:** 2026-11-14

## Context

This project mixes firmware, graphics, optics and mechanical design. Choices in one area constrain the others: screen size affects reflector size, the rendering library limits frame rate, and pin choices clash with the PSRAM. Six months from now, "why did I do it this way?" should have a written answer. The repo is also a portfolio piece, so the reasoning should be visible to readers, not only the result.

## Decision

Record every significant decision as a short ADR in `docs/adr/`, numbered in order and using [template.md](template.md). ADRs are append-only: a changed decision gets a new ADR that supersedes the old one.

## Alternatives considered

| Option | Why not |
|---|---|
| Comments in the code | Explain *what*, rarely *why not the other option*, and get lost in refactors |
| One big design doc | The rejected alternatives and the timeline get overwritten as the doc changes |

## Consequences

- A little writing overhead for each decision.
- Hardware decisions marked **Proposed** must be confirmed or superseded after the physical test that settles them.
