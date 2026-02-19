# Agent Handbook

This page is the navigation map for all agent-facing instructions in PolySynth.

## Source of Truth Order

When instructions overlap, use this order:

1. Root agent policy: [`AGENTS.md`](../../AGENTS.md)
2. Operational rules: [`.agent/rules/`](../../.agent/rules)
3. Operational workflows: [`.agent/workflows/`](../../.agent/workflows)
4. Strategy/process docs: [`plans/active/03_agentic_workflow.md`](../../plans/active/03_agentic_workflow.md)

## What To Read For Common Tasks

- **Run/build/test/lint commands**:
  - [`AGENTS.md`](../../AGENTS.md)
  - [`Justfile`](../../Justfile)
- **Feature delivery loop**:
  - [`.agent/workflows/feature-complete.md`](../../.agent/workflows/feature-complete.md)
- **Open local demo quickly**:
  - [`.agent/workflows/open-demo.md`](../../.agent/workflows/open-demo.md)
- **Architecture constraints**:
  - [`.agent/rules/architecture.md`](../../.agent/rules/architecture.md)
  - [`docs/architecture/DESIGN_PRINCIPLES.md`](../architecture/DESIGN_PRINCIPLES.md)
- **Testing expectations**:
  - [`.agent/rules/testing.md`](../../.agent/rules/testing.md)
  - [`docs/architecture/TESTING_GUIDE.md`](../architecture/TESTING_GUIDE.md)

## Execution Convention

- Use `just` as the primary command surface.
- Prefer recipes listed in `just --list` over ad-hoc command chains.
- Full logs live in `.artifacts/logs/<run-id>/`; use `just logs-latest` for triage.

Common fast paths:

```bash
just quick
just quick-pr
just quick-desktop
just quick-ui
just quick-logs
```

## Related Docs

- Docs hub: [`docs/README.md`](../README.md)
- Planning hub: [`plans/README.md`](../../plans/README.md)
- Active plans: [`plans/active/README.md`](../../plans/active/README.md)
