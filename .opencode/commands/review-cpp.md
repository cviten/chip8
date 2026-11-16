---
description: Review the current chip8 diff for UB, opcode, and lifetime bugs
agent: cpp-reviewer
subagent: true
---

Review the current working-tree changes for $ARGUMENTS. If no scope is given,
review everything in `git diff` plus untracked files under `src/`, `include/`,
and `tests/`.

Report blocking findings first, then should-fix, then a short nit list. Cite
`path:line` for every finding and state what you inspected.
