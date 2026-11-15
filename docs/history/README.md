# History

Documents kept for the record. They are not reference: they describe what was planned and done at
the time, and are left as written (statuses, pins and file names included). Each file name carries
the month and year it belongs to.

## November 2026 - sunlight's master plan, Scarab's first big refactoring with an AI partner

Scarab's first big refactoring done with an AI (Claude) as a development partner: integrating
sunlight's master plan (2026-10-21 to 2026-11-14), on the branch `master_plan`, in step with
sunlight (the engine) and Caravellius (the game built on Scarab). Every item had its own branch,
pull request and tests; the owner reviewed each one before it was merged, and checked by eye what no
test can see (drawing, fullscreen, real gamepads). It was released as Scarab v0.2.0, on sunlight
v0.39.0.

- [master_plan_11_2026.md](master_plan_11_2026.md) - the plan: every item (P, D, B, V, R, S, C and T
  phases) with its decisions, the tests that closed it, and what was found on the way.
- [caravellius_breakages_11_2026.md](caravellius_breakages_11_2026.md) - every change that broke or
  changed something for Caravellius (36 entries), and what it had to do.
