# ADR 0002: the core is headless and clock-free

## Status

accepted

## Context

An emulator needs three things that do not belong in an interpreter: a window,
an audio device, and a 60 Hz clock. Putting any of them in the core makes it
untestable on CI, drags a platform dependency into the library, and forces every
test to stand up a display.

The alternative — a core that owns `SDL_Delay` and a render loop — is how many
first emulators are written, and it is why they cannot be tested.

## Decision

We will keep `chip8` a pure library:

- `step()` executes one instruction. It never waits, sleeps, or reads a clock.
- `tick_timers()` decrements the 60 Hz timers by one. The caller decides when.
- The framebuffer is read through `pixel()`. The core never knows how it is
  drawn.
- Input arrives through `set_key()`. The core has no notion of a keyboard.
- No allocation, no exceptions, and no I/O in `step()`.

## Consequences

- Tests run in microseconds and need no display, so the ASan suite is cheap
  enough to run on every change.
- A front end (SDL, terminal, web) is a thin adapter: run N cycles, tick timers
  at 60 Hz, blit, poll input. None of that logic can regress core tests.
- Frame pacing and audio latency are front-end problems, which is where they
  belong.
- The core cannot enforce speed. A front end that runs too fast is a front-end
  bug, not an emulator bug.

## Alternatives considered

- **Core owns the loop and the window.** Rejected: untestable on CI, and the
  platform dependency leaks into the public API.
- **Expose a `run_frame()` helper in the core.** Rejected for now: it encodes a
  cycle budget per frame (a front-end policy) into the library. Revisit if a
  second front end needs identical pacing.
