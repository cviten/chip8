---
description: Record an architecture decision as a numbered ADR
agent: build
---

Record an architecture decision.

Subject: $ARGUMENTS

Read `docs/adr/` for the next number and match the existing format. Write
`docs/adr/NNNN-<kebab-case-title>.md` with exactly: Status, Context, Decision,
Consequences, Alternatives considered.

Rules:

- Present tense, starting with "We will…". Facts before preferences.
- Alternatives get a real reason, not a strawman.
- Do not edit existing ADRs; supersede them and update the old Status line.
- Keep it under one page, code snippets under ~10 lines.
- If the decision contradicts `AGENTS.md`, say so at the end and ask me to
  update the charter instead of editing it silently.
