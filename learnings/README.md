# Learnings

This folder is a living knowledge base of hard-won lessons from working in this
codebase.  It is written by agents, for agents.

---

## Agent Instructions

> **Read this before working on any task in a topic area covered below.**
> **Write to this after solving a non-trivial problem.**

### When to read

Before starting any task, check whether a relevant learning file exists here.
Reading it takes seconds and avoids repeating mistakes that already cost hours.

### When to write

You MUST add a learning when any of the following happen:

- You make a mistake that costs more than one debug-and-rebuild cycle to fix.
- You discover a non-obvious API difference between a reference (HTML, spec,
  pseudocode) and the actual C++ implementation.
- A pattern you assumed would work silently fails or produces wrong output.
- You find a cleaner or faster way to do something you previously struggled with.
- You make an architectural decision with a non-obvious rationale.

**Do not wait to be asked.** Update the relevant file before ending your session.

### Format

Each learning entry uses this template:

```markdown
## [Short title — what went wrong or what was learned]

**Context**: One sentence on when/where this applies.
**Mistake**: What the wrong approach is (be specific — what code did you write?).
**Fix**: What the correct approach is.
**Why**: Why the correct approach works / why the mistake fails.
**Example** (optional): Minimal before/after code snippet.
```

Entries are grouped by topic file.  Add to an existing file if possible; create
a new file if you are covering a genuinely distinct topic area.

---

## Index

| File | Topic |
|---|---|
| [`html_to_cpp_ui.md`](html_to_cpp_ui.md) | Converting HTML Canvas / CSS prototypes to iPlug2/Skia rendering |
| [`iplug2_threading.md`](iplug2_threading.md) | Audio↔UI thread boundary patterns in iPlug2 |
| [`embedded_pico.md`](embedded_pico.md) | RP2350 Pico embedded patterns, memory constraints, Pico SDK |

---

## Conventions

- Entries are appended, never deleted (add a "Superseded" note if something changes).
- Keep examples short — 5-15 lines maximum.
- Use specific API names, not vague descriptions.
- If a learning belongs to more than one topic, put it in the most relevant file
  and add a one-line cross-reference in the other.
