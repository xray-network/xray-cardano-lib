# TypeScript implementation 0025 instruction

Implementation-Version: v1
Implementation-ID: typescript/0025
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
| `libs/typescript/packages/cip/README.md` | `LOCAL` | Yes | CIP-36 public package scope. |
| `libs/typescript/packages/cip/package.json` | `LOCAL` | Yes | Existing exports and dependency direction. |
| `libs/typescript/packages/cip/src/cip36/metadata.ts` | `LOCAL` | Yes | Registration, deregistration, and metadata-view implementations. |
| `libs/typescript/packages/cip/test/cip36.test.mjs` | `LOCAL` | Yes | Existing tests, signing hashes, and compatibility expectations. |

## Objective

Fix CIP36 delegation weight validation.

Correct the locally reproduced inverted validation predicate: a nonempty weighted delegation is
valid when at least one weight is positive, and invalid when every weight is zero. Preserve
legacy single-key registration behavior, uint32 bounds, and the current cryptographic API.
This is the explicit bug-fix requirement selected from the audit, not a claim of consuming newer
CML source. Current tests accept weight 0 and reject weight 1; both expectations must be corrected.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `C01` | Make `CIP36RegistrationCbor.verify()` reject all-zero weighted delegations and accept any nonempty distribution containing a positive uint32 weight. | Keep empty-list and integer-bound rejection at their current owners; legacy registrations remain accepted. | `libs/typescript/packages/cip/src/cip36/metadata.ts` | Test [0], [0,0], [1], [0,1], [1,0], and uint32 maximum. |
| `C02` | Update incorrectly inverted test fixtures and cover every metadata conversion entry point that invokes verification. Ensure failure occurs before mutating caller metadata. | Keep existing exception family; signatures and hashes for unchanged payloads must remain identical. | `libs/typescript/packages/cip/test/cip36.test.mjs` | Use real synthetic fixture signatures; test verify, to_metadata_bytes, try_into_metadata and add_to_metadata. |

## Implementation steps

1. Add minimal failing tests for all-zero rejection and positive/mixed acceptance using synthetic local keys.
2. Replace the inverted predicate without changing serialization, hashing, signature verification APIs, or the delegation model.
3. Correct old regression expectations, verify legacy/boundary behavior and metadata mutation atomicity, and run the required gates.

## Validation

After a build, run the focused checks from the repository root:

```sh
npm --prefix libs/typescript run build
node --test libs/typescript/packages/cip/test/cip36.test.mjs
npm --prefix libs/typescript run check
```

Record actual results, including a regression that fails before the fix and passes afterward.
The final gate includes browser boundaries and packed-package checks. Validate the affected
SPECTRE record; do not mark the implementation accepted.

## Compatibility and human review

This intentionally changes invalid acceptance and valid rejection in the public metadata-view
validation path. `verify()` remains structural delegation validation; do not silently turn it into
cryptographic signature verification. Preserve all existing signing hashes and wire encodings.
The metadata-preservation plan is independent; neither plan requires the other's result.

## Completion criteria

Every weight case has the required outcome through all verification-dependent entry points;
failed additions leave the caller's metadata unchanged. Legacy behavior, hashing tests and the
full TS gate pass. Record C01–C02 and the portable validation rule.

## Out of scope

Metadata round-trip redesign, unrelated CIP behavior, new public types, crypto dependency changes,
upstream capture, automatic updates, and C++ work.

## Blockers

None for this bounded LOCAL objective. Adoption of newly captured upstream behavior is separate;
see [cardano-multiplatform-lib consumer guidance](../../providers/cardano-multiplatform-lib/PROVIDER.md#maintained-consumer-guidance).
