# TypeScript implementation 0026 instruction

Implementation-Version: v1
Implementation-ID: typescript/0026
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
| `libs/typescript/packages/core/src/cbor/types.ts` | `LOCAL` | Yes | Owned CBOR value and encoding metadata representation. |

## Objective

Preserve complete CIP36 metadata views.

Make registration and deregistration metadata views preserve the complete valid input metadata
map across byte decode/encode, including unrelated labels and compatible CBOR encoding hints.
The local reproduction decoded labels 7/61284/61285 and emitted only 61284/61285, changing an
indefinite outer map into a definite map. Restore the repository's lossless-data requirement
without changing signing preimages or weakening validation.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `C01` | Retain a defensive full metadata representation in both view types; unchanged from_metadata_bytes/to_metadata_bytes round-trips preserve exact bytes, order, widths, outer container form, and nested encoding choices. | Reject currently invalid reserved fields and duplicate labels as before; do not broaden inner CIP36 schemas. | `libs/typescript/packages/cip/src/cip36/metadata.ts` | Byte-exact registration and deregistration round-trips with extra labels, shuffled order, nonminimal widths and indefinite containers. |
| `C02` | Make try_from_metadata/try_into_metadata retain unrelated labels using the chain-owned Metadata type. Keep add_to_metadata as a merge of the view’s CIP36 fields into caller metadata, preserving caller-owned unrelated labels. | No competing Metadata class or new public method; fresh constructed views retain deterministic output. | `libs/typescript/packages/cip/src/cip36/metadata.ts` | Test view conversions, caller independence, unrelated-label collisions and fresh construction. |
| `C03` | Keep hash_to_sign and witness preimages limited to the current registration/deregistration payload; unrelated labels must not affect them. Keep canonical output explicit and separate from preserved output. | Existing key, address, signature, and metadata nominal owners remain unchanged. | `libs/typescript/packages/cip/test/cip36.test.mjs` | Assert identical signing hash with/without unrelated metadata and canonical-equivalent results for noncanonical input. |

## Implementation steps

1. Add failing lossless round-trip cases using legacy registration and deregistration so this plan does not depend on the weight-validation fix.
2. Retain and defensively clone the owned CBOR representation; preserve compatible nested hints and keep constructor output deterministic.
3. Cover Metadata object conversions and the explicitly retained add_to_metadata merge contract; verify no signing-preimage expansion.
4. Run focused tests and the full TS gate.

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

This expands what a decoded view retains, not what its reserved fields accept. Additional
metadata is carried as existing transaction metadatum values. The current JSON view shape remains
unchanged; full-fidelity CBOR/Metadata round-tripping does not imply encoding metadata in JSON.
No setter semantics or optional voting-purpose behavior changes are included.

## Completion criteria

Both metadata view types round-trip valid complete metadata byte-for-byte; Metadata conversions
retain extra labels; caller buffers and objects cannot mutate stored state. Hash/signature
fixtures, canonical comparisons and the full TS gate pass. Record C01–C03.

## Out of scope

Weight validation, voting-purpose setter fixes, arbitrary unknown inner CIP36 fields, JSON API
expansion, new nominal metadata types, automatic upstream adoption, and C++ work.

## Blockers

None for this bounded LOCAL objective. Adoption of newly captured upstream behavior is separate;
see [cardano-multiplatform-lib consumer guidance](../../providers/cardano-multiplatform-lib/PROVIDER.md#maintained-consumer-guidance).
