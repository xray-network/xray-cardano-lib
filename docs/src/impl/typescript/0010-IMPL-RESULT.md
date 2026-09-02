# TypeScript implementation 0010 result

Result-Version: v1
Implementation-ID: typescript/0010
Instruction: ./0010-IMPL-INSTR.md
Evidence-Mode: HYBRID

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `C001` | Implemented | Added exact CIP-67 label encoding/decoding and labeled asset-name split/join in the focused proposal subpath. | Official-vector and strict prefix/content cases pass. |
| `C002` | Implemented | Added CIP-68 asset relationships, strict three-field datums, class/version metadata, v4 nesting, URI/chunk, duplicate, and resource validation. | Class/version, datum, metadata, URI, and ownership tests pass. |
| `C003` | Implemented | Kept general CIP-67 out of stable roots while exporting active CIP-68 identically through CIP/runtime paths. | Import/browser/packed-consumer gates pass. |

## Outcome

The TypeScript CIP package now supports exact provisional labels and active CIP-68 relationships/datums while preserving existing `AssetName`, `ScriptHash`, and `PlutusData` owners.

## Inputs consumed

- `typescript/0003` and `typescript/0004` accepted results
- `0001-cardano-cips` snapshot and captured CIP-67 document/registry/schema and CIP-68 document
- Existing TypeScript CIP, chain, Plutus, runtime, package, and documentation owners

## Project changes

- Added focused `cip67` and `cip68` modules and package exports.
- Added label, relationship, datum, metadata, URI, duplicate, and limit tests.
- Added proposal-aware root/runtime exports and documentation.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `C001` | CIP-67 prefixes are exact four-byte bracket/CRC labels. | Additive proposal API. | Import the explicit `cip67` subpath. |
| `C002` | CIP-68 enforces policy/content relationships and normative datum/metadata shapes. | Additive active API. | Reject stale two-field datums and invalid class/version metadata. |
| `C003` | CIP-68 is stable-root/runtime visible; CIP-67 remains focused. | Additive exports. | Do not depend on general CIP-67 aggregate exposure. |

## Validation

`npm --prefix libs/typescript run check` passed: TypeScript build, 184 tests, and packed ESM/NodeNext/bundler consumers (566 files, 2,599,775 unpacked bytes).

## Deviations from instruction

None.

## Remaining human review

Review registry/vector fidelity, proposal isolation, the class/version matrix, v4 metadata selection, URI/resource limits, and export identity before acceptance.

## Reproducibility

Run `npm --prefix libs/typescript run check` from the repository root.
