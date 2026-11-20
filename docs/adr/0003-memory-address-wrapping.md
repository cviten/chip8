# ADR 0003: memory addresses wrap modulo 4096

## Status

accepted

## Context

CHIP-8 has a 12-bit address space (`0x000`..`0xFFF`); `I` and the program
counter are 12-bit registers. Several operations add an offset to `I`
(`FX1E`, `FX33`, `FX55`, `FX65`, `DXYN`) and `BNNN` adds `V0` to `NNN`, so the
result can exceed `0xFFF`. The core currently indexes its 4096-byte `std::array`
directly, which is undefined behaviour for such an address. ADR 0001 fixes
opcode semantics but says nothing about what happens at the top of memory.

## Decision

Every emulated memory access wraps modulo `kMemorySize` (4096, i.e. 12 bits):

- `fetch_opcode()` reads `PC` and `PC + 1` modulo 4096.
- `DXYN` reads sprite rows at `I + row` modulo 4096.
- `FX33` writes at `I`, `I + 1`, `I + 2` modulo 4096.
- `FX55` / `FX65` transfer at `I + index` modulo 4096.
- `FX1E` stores `I + VX` as a 12-bit value, and `BNNN` stores `NNN + V0` as a
  12-bit value.
- Normal and skip `PC` advances wrap modulo 4096.

The public `peek()` accessor is not an emulated access. It keeps its `assert`
and remains a debug/test API whose caller must pass a valid address, so a test
that reads the wrong address still fails loudly.

Stack overflow and underflow are not address wrapping: `push()` and `pop()`
keep counting those cases in `unsupported_opcodes()`.

## Consequences

- No index into `memory_` can fall outside the array; behaviour is defined and
  deterministic.
- A ROM that runs off the top of memory continues at `0x000`, matching 12-bit
  hardware instead of trapping or corrupting memory.
- Wrapped writes can overwrite low memory, including the font, exactly as on
  the original machine.
- Tests can assert exact wrapped addresses.
- `peek()` still catches invalid addresses in tests.

## Alternatives considered

- **Clamp to `0xFFF`.** Rejected: it discards the low address bits and does not
  match 12-bit hardware.
- **Treat the access as an unsupported opcode.** Rejected: the opcode is valid;
  only the address is out of range, and a real ROM can reach it.
- **Leave the access unchecked.** Rejected: undefined behaviour, and a silent
  out-of-bounds write can corrupt unrelated state.
