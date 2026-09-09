# TypeScript implementation 0028 instruction

Implementation-Version: v1
Implementation-ID: typescript/0028
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
| `libs/typescript/packages/plutus/src/blueprint/index.ts` | `LOCAL` | Yes | Existing local-reference resolution and schema/value validation. |
| `libs/typescript/packages/plutus/test/blueprint.test.mjs` | `LOCAL` | Yes | Current blueprint compatibility and security tests. |

## Objective

Cover generic blueprint definition references.

Add regression coverage proving that the existing blueprint parser and value validator resolve
literal definition keys containing generic/tuple notation and JSON Pointer escapes. Definition
identity comes from the key, not schema title. This tests the locally owned API requirements from
the audit; it does not import an uncaptured CIP revision or require a new naming parser.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `C01` | Cover literal generic, nested generic, tuple and qualified definition keys through both blueprint parsing and validate_blueprint_value. | Treat names as opaque keys; no angle-bracket type parser or renaming. | `libs/typescript/packages/plutus/test/blueprint.test.mjs` | Use Option<Int>, Option<cardano/address/StakeCredential>, Tuple<<Int,ByteArray>> and nested generic keys. |
| `C02` | Cover ~1 and ~0 escapes, their decode order, duplicate JSON keys, unresolved references, remote references, and simple/parameterized/mismatched titles. | Preserve document-local resolution, safe-property rejection, immutability and recursion limits. | `libs/typescript/packages/plutus/test/blueprint.test.mjs` | Positive and negative schema/value pairs; include a literal ~1 in a key to detect double-decoding. |

## Implementation steps

1. Construct small original local fixtures with integer/bytes/constructor/list schemas and the named definition keys.
2. Exercise parsed validator redeemers, datum schemas and parameter schemas through the public APIs; prove wrong values still fail.
3. Keep this change test-only. If a runtime defect appears, record the exact failing case and create a separate bounded plan rather than silently broadening this one.
4. Run the focused blueprint tests and full workspace gate.

## Validation

From the repository root:

```sh
npm --prefix libs/typescript run build
node --test libs/typescript/packages/plutus/test/blueprint.test.mjs
npm --prefix libs/typescript run check
```

Verify all table cases execute, including negative value validation after a successful reference
resolution. No runtime behavior change is expected. Validate the affected SPECTRE record.

## Compatibility and human review

Preserve the current accepted blueprint vocabulary, v1/v2/v3 scope, title handling, compiled
script hash checks, error boundaries, limits and browser-safe dependency graph. These tests do not
assert complete compliance with a newer uncaptured CIP-57 document.

## Completion criteria

Tests protect generic/tuple/qualified definition identity and both escaping steps through parser
and value-validation paths, and demonstrate rejection of invalid references and values. The
complete TS gate passes without production-source changes. Record C01–C02.

## Out of scope

Remote schema loading, generic JSON Schema support, source changes, type-expression parsing,
new blueprint language versions, upstream evidence capture/adoption, and C++ work.

## Blockers

None for this bounded LOCAL objective. Adoption of newly captured upstream behavior is separate;
see [cardano-cips consumer guidance](../../providers/cardano-cips/PROVIDER.md#maintained-consumer-guidance).
