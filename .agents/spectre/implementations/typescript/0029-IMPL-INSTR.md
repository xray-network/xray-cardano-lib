# TypeScript implementation 0029 instruction

Implementation-Version: v1
Implementation-ID: typescript/0029
Created: 20260909T114243Z
Evidence-Mode: LOCAL
Depends-On: NONE
Provider-Evidence: NONE

## Inputs and authority

All code-formatted repository paths are repository-root-relative. Inputs describe local requirements
and owned files. The preceding upstream audit motivates this plan; uncaptured upstream material is
not a normative implementation input. No new upstream conformance claim follows from completing it.

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| Current human request (2026-09-09): plan the preceding provider-audit recommendations; the bounded local requirement is reproduced in Objective | `LOCAL` | Yes | User-requested behavior and scope. |
| `AGENTS.md` | `LOCAL` | Yes | Repository ownership, lifecycle, and validation rules. |
| `docs/src/adr/typescript/0001-lossless-cbor-and-encoding-metadata.md` | `LOCAL` | Yes | Preserved versus canonical encoding and mutation compatibility. |
| `docs/src/adr/typescript/0003-upstream-evidence-and-package-ownership.md` | `LOCAL` | Yes | Package ownership and evidence boundaries. |
| `libs/typescript/README.md` | `LOCAL` | Yes | Maintained workspace and completion gate. |
| `libs/typescript/package.json` | `LOCAL` | Yes | Build, test discovery, and package smoke commands. |
| `libs/typescript/packages/plutus/README.md` | `LOCAL` | Yes | Plutus ownership and bounded public API. |
| `libs/typescript/packages/plutus/package.json` | `LOCAL` | Yes | Plutus package exports and dependencies. |
| `libs/typescript/packages/plutus/test/conformance.test.mjs` | `LOCAL` | Yes | Existing corpus reader, integrity checks and baseline test entry point. |
| `libs/typescript/packages/plutus/test/flat.test.mjs` | `LOCAL` | Yes | Owned Flat codec test patterns. |
| `libs/typescript/tools/run-tests.mjs` | `LOCAL` | Yes | Workspace test discovery. |

## Objective

Support versioned UPLC conformance layouts.

Prepare the test-only corpus reader for both historical text-only fixtures and newer text/Flat
file layouts, without switching the currently pinned corpus or changing runtime semantics.
Use original local miniature fixtures to specify and test transport behavior. Keep the existing
3,013-entry/1,003-vector baseline checks attached to its existing explicit snapshot selection;
future snapshots need their own separately reviewed exact selection and case accounting.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `C01` | Extract a test-only loader/runner with explicit corpus bytes and expected inventory configuration; keep baseline invocation pinned to the existing path and counts. | No latest-snapshot discovery, network fetch, runtime export or package dependency. | `libs/typescript/packages/plutus/test/conformance.test.mjs` | Existing baseline still executes exactly 1,003 cases; missing entries or altered expected counts fail. |
| `C02` | Keep binary content as bytes; dispatch .uplc to text parsing and .flat to Flat decoding. Support .uplc.expected/.flat.expected outcomes and both <case>.uplc.budget.expected and <case>.budget.expected budget naming, with deterministic ambiguity rejection. | Never decode Flat bytes through UTF-8 or guess serialized-script envelopes. | `libs/typescript/packages/plutus/test/conformance.test.mjs` | Original miniature text/Flat pairs, non-UTF-8 bytes, historical/new budget names and conflicting files. |
| `C03` | Handle parse error and parse/decode error as codec-stage expectations, separate from evaluation failure and success. Validate text results semantically and Flat expected results via the owned Flat decoder; compare budgets explicitly. | An evaluator exception must not satisfy a parse/decode-failure expectation. | `libs/typescript/packages/plutus/test/conformance.test.mjs` | Success, codec failure, evaluator failure, wrong result, wrong budget and malformed expectation cases. |
| `C04` | Validate unique safe paths, canonical base64, sizes and SHA-256 before building lookup maps; require every executable case to have a disposition and every referenced expectation to exist. | Preserve old strict inventory checks and refuse unknown executable formats or silently skipped failures. | `libs/typescript/packages/plutus/test/conformance.test.mjs` | Duplicate/path-escape/base64/hash/size/missing-result/missing-budget failures; complete accounting for mixed layouts. |

## Implementation steps

1. Extract minimal reusable test helpers below `packages/plutus/test/support/` without changing the baseline selection or runtime owners.
2. Add original miniature corpus tests below `packages/plutus/test/` for both layouts, binary fidelity, integrity errors and stage-specific outcomes.
3. Apply explicit expected-count configuration to the existing baseline invocation; keep immutable corpus bytes and their source identity unchanged.
4. Run all current official baseline cases plus helper tests and the full workspace gate.

## Validation

From the repository root:

```sh
npm --prefix libs/typescript run build
node --test libs/typescript/packages/plutus/test/conformance.test.mjs libs/typescript/packages/plutus/test/conformance-layout.test.mjs
npm --prefix libs/typescript run check
```

Create `conformance-layout.test.mjs` as the focused miniature-corpus test. Existing baseline
fixtures remain existing regression inputs, not a newly selected semantic dependency of this
LOCAL test-infrastructure change. Preserve their current exact-count/hash checks and report the
baseline and miniature-layout results separately. Validate the affected SPECTRE record.

## Compatibility and human review

Only the test harness changes. Keep protocol majors 5–11, UPLC 1.0/1.1, language and builtin
availability, costs, and runtime parsing/evaluation unchanged. Completing this plan proves reader
compatibility, not that the 1.68 corpus passes. Do not copy official fixtures into package tests;
miniature transport fixtures must be locally authored and identified as such.

## Completion criteria

Historical and modern miniature layouts are validated without losing binary bytes; stage and
budget expectations fail correctly; malformed/incomplete inventories fail closed. The existing
1,003-case baseline and complete TS gate still pass. Record C01–C04; report actual new-snapshot
adoption as outstanding rather than claiming it completed.

## Out of scope

Fetching or capturing upstream artifacts, switching snapshot paths, modifying evaluator/parser
semantics or cost models, copying upstream fixtures into packages, Plutus V4/protocol 12/new
builtins, permissive skips, scheduling, and C++ work.

## Blockers

None for this bounded LOCAL objective. Adoption of newly captured upstream behavior is separate;
see [uplc consumer guidance](../../providers/uplc/PROVIDER.md#maintained-consumer-guidance).
