---
description: Audit a file for undefined behaviour and out-of-bounds indexing
agent: cpp-reviewer
subagent: true
---

Audit $1 for undefined behaviour, including code outside the current diff.

Check: memory index bounds (especially `I + offset` from ROM data), signed and
unsigned mixing, shifts, integer overflow, uninitialised reads, `std::array`
indexing without proof, iterator invalidation, and anything that could make
`step()` read outside the 4096-byte address space.

For each finding: `path:line`, the rule violated, the ROM bytes or call sequence
that triggers it, and a one-line fix. Rank by whether a real ROM could hit it.
Say plainly if the file looks clean and name what you verified.

$ARGUMENTS
