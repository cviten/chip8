# chip8

A headless CHIP-8 interpreter in C++17, with a doctest suite and a sanitizer
preset. The core executes instructions and owns the framebuffer; it has no
window, no audio device, and no clock.

**Status:** the base CHIP-8 instruction set is implemented and tested (18 cases,
90 assertions). There is no front end yet, so nothing draws a window.

## Build

Requires CMake >= 3.21, Ninja, and a C++17 compiler. doctest is vendored in
`third_party/`, so there is no network access at configure time.

```sh
cmake --preset dev
cmake --build --preset dev -j
ctest --preset dev --output-on-failure
```

Sanitizers:

```sh
cmake --preset asan && cmake --build --preset asan -j && ctest --preset asan
cmake --preset tsan && cmake --build --preset tsan -j && ctest --preset tsan
```

On kernels with high ASLR entropy (`vm.mmap_rnd_bits` >= 32), TSan aborts with
`FATAL: ThreadSanitizer: unexpected memory mapping`. That is an environment
issue, not a project defect; run it with ASLR disabled:

```sh
setarch "$(uname -m)" -R ./build/tsan/chip8_tests
```

TSan is dormant until the project actually has threads, which the charter
forbids.

Warnings are errors in every preset (`-Wconversion`, `-Wsign-conversion`,
`-Wold-style-cast`, and friends). If your compiler is older than GCC 11 or
Clang 13, expect flag differences.

## Layout

```
include/chip8/emulator.hpp   public API
src/emulator.cpp             fetch/decode/execute, font, framebuffer
tests/emulator_test.cpp      doctest cases, driven through load() + step()
docs/adr/                    decisions that are expensive to reverse
third_party/doctest/         vendored single-header doctest 2.4.9
```

## Design

Two decisions shape everything; read them before changing the core:

- [`0001-opcode-semantics.md`](docs/adr/0001-opcode-semantics.md) — where the
  reference interpreters disagree, this project follows the original one.
  `8XY6`/`8XYE` shift `VX`, `FX55`/`FX65` leave `I` alone, `DXYN` wraps.
- [`0002-headless-core.md`](docs/adr/0002-headless-core.md) — the core never
  sleeps, never reads a clock, never allocates in `step()`.

The intended front-end loop:

```cpp
chip8::Emulator emulator;
emulator.load_rom(rom);

while (running) {
  for (int i = 0; i < cycles_per_frame; ++i) {
    emulator.step();
  }
  emulator.tick_timers();  // 60 Hz
  blit(emulator);          // read via pixel(x, y)
  poll_input(emulator);    // set_key(key, down)
}
```

`cycles_per_frame` is front-end policy, not core state. A typical value is
10–15 for ~700–900 Hz at 60 fps.

## Opcode status

| Range | Status |
| --- | --- |
| `00E0`, `00EE` | implemented |
| `0NNN` | ignored, counted as unsupported (RCA 1802) |
| `1NNN`, `2NNN`, `3XNN`, `4XNN`, `5XY0`, `9XY0` | implemented |
| `6XNN`, `7XNN` | implemented |
| `8XY0`–`8XY7`, `8XYE` | implemented |
| `ANNN`, `BNNN`, `CXNN`, `DXYN` | implemented |
| `EX9E`, `EXA1` | implemented |
| `FX07`, `FX0A`, `FX15`, `FX18`, `FX1E`, `FX29`, `FX33`, `FX55`, `FX65` | implemented |
| SCHIP / XO-CHIP (`00FB`, `FX75`, `5XY2`, `FN01`, …) | out of scope |

## Next steps

Natural increments, each one target and one test suite:

1. **CLI runner** — load a ROM, run N cycles, print the framebuffer as ASCII.
   This makes manual ROM debugging possible without a GUI.
2. **ROM smoke tests** — a tiny hand-assembled ROM per opcode group, run to
   completion in CI.
3. **SDL front end** — separate target, depends on `chip8::chip8` only.
4. **Disassembler** — same decoder, different consumer; keep it out of the core
   until the second consumer exists.

## Working on this with OpenCode

```sh
opencode
```

Then:

| Command | Use |
| --- | --- |
| `/add-opcode 8XY4` | implement one opcode with tests and ADR note |
| `/verify` | build, test, and sanitizer-check the current change |
| `/review-cpp` | read-only review of the working tree |
| `/ub-audit src/emulator.cpp` | targeted undefined-behaviour audit |
| `/explain-error` | diagnose a compiler or linker failure |
| `/adr "title"` | record a decision |

`AGENTS.md` holds the binding rules, including the "no allocation in `step()`"
contract and the warning policy. Agents live in `.opencode/agents/`; the
reviewer and test writer cannot touch `src/`.

ROMs are not committed — `roms/` is ignored. Supply your own.
