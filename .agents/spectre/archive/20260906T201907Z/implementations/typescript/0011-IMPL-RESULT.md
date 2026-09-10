# TypeScript implementation 0011 result

Result-Version: v1
Implementation-ID: typescript/0011
Instruction: ./0011-IMPL-INSTR.md
Evidence-Mode: HYBRID

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `C001` | Implemented | Added preserved-CBOR diagnostics for canonical widths/lengths/order, definite containers, tag-258 consistency, integer bounds, collection counts, and the 4096-detail cap. | Encoding and cap cases pass in the full suite. |
| `C002` | Implemented | Added captured body, output, multiasset, certificate, pool interaction, withdrawal, vote, optional, and Catalyst restrictions. | Advisory rule and mode cases pass. |
| `C003` | Implemented | Published the optional function through the focused module and scoped `cip21` namespaces without an aggregate top-level function. | Build, imports/browser checks, and packed consumers pass. |

## Outcome

TypeScript applications can obtain stable, sorted, bounded CIP-21 compatibility findings from preserved Conway transaction bytes without mutation or implicit enforcement.

## Inputs consumed

- `typescript/0004` accepted result
- `0001-cardano-cips` snapshot and captured active CIP-21 document
- Existing TypeScript core CBOR, Conway transaction, CIP/runtime package, and documentation owners

## Project changes

- Added the focused `cip21` diagnostic module.
- Added raw-CBOR encoding, semantic restriction, Catalyst, ordering, and cap tests.
- Added scoped CIP/runtime exports and advisory-boundary documentation.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `C001` | Findings inspect preserved encoding and are stable/sorted/bounded. | Additive read-only API. | Keep decoded source bytes available. |
| `C002` | Captured device restrictions are reported independently of ledger validity. | No builder/decoder behavior change. | Apply caller-specific policy to findings. |
| `C003` | Diagnostics are namespace-scoped and optional. | Additive exports. | Call `cip21.diagnose_cip21_transaction` explicitly. |

## Validation

`npm --prefix libs/typescript run check` passed: TypeScript build, 184 tests, and packed ESM/NodeNext/bundler consumers (566 files, 2,599,775 unpacked bytes).

## Deviations from instruction

None.

## Remaining human review

Review stable code/path coverage, preserved-CBOR inspection, pool/Catalyst interactions, diagnostic truncation, and export scoping before acceptance.

## Reproducibility

Run `npm --prefix libs/typescript run check` from the repository root.
