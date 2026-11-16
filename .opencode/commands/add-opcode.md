---
description: Implement one CHIP-8 opcode with full flag coverage and tests
agent: build
---

Implement opcode `$1` in the core.

1. Read `docs/adr/0001-opcode-semantics.md` first. If the reference
   interpreters disagree on this opcode and the ADR does not cover it, add the
   row before writing code.
2. Decode the fields with the existing `x`, `y`, `n`, `nn`, `nnn` locals at the
   top of `execute()`. Do not re-derive nibbles inside the branch.
3. Implement in the correct `switch` branch. The `default` paths must keep
   incrementing `unsupported_opcodes()`.
4. Update the status table in `README.md`.
5. Add tests covering: the normal path, each flag outcome, one boundary
   (`0x00`/`0xFF`/wrap), and PC advancing exactly once.

Then run:

```sh
cmake --build --preset dev -j
ctest --preset dev --output-on-failure
cmake --preset asan && cmake --build --preset asan -j && ctest --preset asan --output-on-failure
```

Report: the semantics you implemented, the exact flag write, the tests added,
the commands run, and anything you could not verify.
