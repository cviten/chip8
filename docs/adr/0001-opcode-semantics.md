# ADR 0001: opcode semantics follow the original CHIP-8 interpreter

## Status

accepted

## Context

CHIP-8 has no single specification. The original interpreter, CHIP-48, SCHIP,
and XO-CHIP disagree on several opcodes, and most public ROMs were written
against one particular behaviour. An emulator that mixes behaviours passes
some ROMs and corrupts others, and the bug appears as a corrupted sprite rather
than a crash — expensive to trace back to an opcode.

This project targets the original CHIP-8 set only. SCHIP and XO-CHIP are out of
scope until an ADR says otherwise.

## Decision

We will implement the following semantics, and every new opcode will document
its choice here when the reference interpreters disagree:

| Opcode | Behaviour | Why |
| --- | --- | --- |
| `8XY6` / `8XYE` | Shift `VX` in place; `VY` is ignored | Original interpreter; the "shift VY" reading is a CHIP-48 change |
| `FX55` / `FX65` | `I` is unchanged after the transfer | Original interpreter; SCHIP increments `I` |
| `DXYN` | Coordinates wrap modulo 64 and 32; pixels XOR; `VF` = 1 on any collision and is cleared before drawing | Original interpreter; clipping is a later variant |
| `BNNN` | Jump to `NNN + V0` | Original interpreter; `BXNN` uses `VX` |
| `0NNN` | Not executed; counted in `unsupported_opcodes()` | Targets the RCA 1802, which no ROM needs |
| `CXNN` | Fixed PRNG seed | Deterministic tests and reproducible runs |

## Consequences

- ROMs written for SCHIP-only behaviour will misdraw. That is acceptable: they
  are out of scope, and the alternative is an ambiguous core.
- Tests can assert exact flag values and pixel positions, including collision.
- Changing any row above is a visible, reviewable event rather than a silent
  drift, and any affected test must change in the same commit.

## Alternatives considered

- **Follow whichever variant the ROM needs at runtime.** Rejected: it requires
  detection heuristics, doubles the test matrix, and hides the decision from
  the reader.
- **Implement SCHIP now.** Rejected: it adds 128x64 mode, `FX75`/`FX85` flags,
  and plane-based drawing before a single original ROM runs.
- **Seed the RNG from `std::random_device`.** Rejected: makes `CXNN` tests
  non-deterministic for no benefit; games only need plausible variation.
