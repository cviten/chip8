---
description: Explain a compiler or linker error and give the smallest correct fix
agent: build
---

Explain this build failure:

$ARGUMENTS

Do not start editing. Tell me:

1. The **first** real error and its instantiation chain, at most 15 lines.
2. The root cause in one or two sentences: missing include, ambiguous overload,
   narrowing warning promoted to an error by `-Werror`, template constraint,
   or link-time mismatch.
3. Whether it is my code, a CMake target, or a warning flag.
4. The smallest fix, and the smallest fix that is also the right fix if they
   differ.

Never propose `// NOLINT`, `-Wno-*`, or a cast that only silences the diagnostic.
If the pasted text is truncated, ask for the specific lines instead of guessing.
