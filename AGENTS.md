# Project instructions

`chip8` is a headless CHIP-8 interpreter in C++17. This file is the charter:
read it before your first edit.

## Decisions

Changing a row is a migration, not an edit. Record it as an ADR.

| Decision | Choice |
| --- | --- |
| Language standard | C++17 — no `std::span`, `std::expected`, `std::jthread`, `<format>` |
| Build | CMake >= 3.21 + Ninja, `CMakePresets.json` |
| Tests | doctest 2.4 (vendored in `third_party/`) via CTest |
| Packages | none — no third-party runtime dependency, ever, without an ADR |
| Layout | `include/chip8/` public, `src/` private, `tests/` |
| Errors | `LoadError` enum; exceptions only from the standard library |
| Deliverable | static library `chip8`; the front end is a separate consumer |
| Threading | none; the core is single-threaded by contract |
| Timing | the core owns no clock; the caller drives it |
| Determinism | fixed RNG seed, no I/O, no wall-clock reads in the core |

## Toolchain

```sh
cmake --preset dev                                  # configure (Ninja, Debug, -Werror)
cmake --build --preset dev --target chip8_tests -j  # build one target
ctest --preset dev --output-on-failure -R <regex>   # run matching tests
cmake --preset asan && cmake --build --preset asan -j && ctest --preset asan
```

`CHIP8_WARNINGS_AS_ERRORS` is ON in every preset. A warning is a build failure,
not a note. Never run a clean rebuild or delete `build/` unless asked.

## Domain contract

- The core is **headless**: no window, no audio, no SDL, no main loop. It must
  keep building on a machine with no display server.
- `step()` executes exactly one instruction, advances PC at most once, and
  **must not allocate, throw, log, or read a clock**.
- The caller runs `step()` at roughly 500–1000 Hz and `tick_timers()` at 60 Hz.
  Never introduce a timer inside the core.
- The framebuffer is 64x32, row-major, one byte per pixel, addressed as
  `y * 64 + x`. Only `pixel()` and `clear_display()` touch it.
- Opcode semantics are fixed by `docs/adr/0001-opcode-semantics.md`. If you
  believe one is wrong, write an ADR — do not silently follow a different
  reference interpreter.
- Any opcode that is not implemented must increment `unsupported_opcodes()`.
  Never let it become a silent no-op.
- ROMs are copyrighted and must not be committed or downloaded. `roms/` is
  ignored.

## Definition of done

1. `chip8` and `chip8_tests` build with zero warnings (`-Werror`).
2. New behaviour has a doctest test; a bug fix has a test that failed first.
3. `ctest --preset dev` passes; `asan` passes for anything touching memory,
   arithmetic, or the framebuffer.
4. `clang-format` applied to changed lines only.
5. If an opcode was added: a test per flag path (carry, borrow, collision) and,
   for ambiguous semantics, an ADR entry.
6. `git diff` has no unrelated edits.

Never report a gate as passing unless you ran it and can show the output.

## Reporting

- Compiler/template errors: the first error plus its instantiation chain, at
  most ~30 lines, then the root cause. Never paste full spew.
- Reference `path:line`; do not echo whole files.
- When adding an opcode, state which nibbles you extracted and which flag you
  wrote, since off-by-one mask errors are the dominant bug class here.
- Say what you did not verify.

## Correctness

- No undefined behaviour to silence a warning. No `reinterpret_cast`,
  `const_cast`, or C-style cast without a comment explaining why it is safe.
- All arithmetic on 8-bit values goes through an explicit cast; `-Wconversion`
  and `-Wsign-conversion` are on, and suppressing them is forbidden.
- Every memory index is bounds-checked or provably in range. `peek()` asserts;
  `pixel()` returns false out of range.
- Opcode fields are decoded once, at the top of `execute()`. Do not re-derive
  nibbles inline in a case branch.
- Advance PC exactly once per instruction, and only in `step()`. Jump and skip
  instructions assign PC; they never add to an already-updated PC twice.

## Ownership and lifetime

- Rule of zero. No owning raw pointers, no `new`/`delete`, no manual `malloc`.
- The interpreter holds all state by value in `std::array`; there is no heap
  allocation after construction.
- `load_rom` copies; it never stores the caller's pointer.
- No `std::shared_ptr` in this project. If you think you need one, the design
  is wrong.

## Testing with doctest

- `TEST_CASE("...")` per behaviour; `SUBCASE` for variations of the same setup.
- `REQUIRE` for anything needed by later assertions, `CHECK` otherwise.
- Helper functions that use `REQUIRE` must return `void`.
- No `sleep`, no clock, no randomness in tests: the RNG seed is fixed, so
  `CXNN` results are stable.
- Test opcodes through `load()` + `step()`, not by calling `execute()`.

## Never without explicit permission

- Add a dependency, raise the standard, or vendor a new header.
- Edit generated output: `build*/`, `compile_commands.json`.
- Run `git push`, `git reset --hard`, `git clean`, or history rewrites.
- Commit or download a ROM.
- Disable warnings, add `// NOLINT`, or change a preset's sanitizer flags.
- Reformat files you did not otherwise change.
