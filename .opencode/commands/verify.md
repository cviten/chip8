---
description: Build chip8 and its tests, run them, and report honestly
agent: build
---

Verify the current change. Do not fix anything unless I ask.

Recent diff:
!`git diff --stat`

Steps:

1. Build both targets: `cmake --build --preset dev -j`
2. Run the narrowest matching tests: `ctest --preset dev --output-on-failure -R <regex>`
   If the change touches memory, indexing, arithmetic, or the framebuffer, run
   the `asan` preset as well.
3. If the change touches an opcode, also run the full suite — opcode bugs show
   up as regressions elsewhere.

Report, in order: the exact commands, pass or fail with the first failing
doctest case and its assertion, any new compiler warning on a line I touched,
and an explicit list of what you did not verify.

$ARGUMENTS
