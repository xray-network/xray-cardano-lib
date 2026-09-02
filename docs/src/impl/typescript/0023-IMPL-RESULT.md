# TypeScript implementation 0023 result

Result-Version: v1
Implementation-ID: typescript/0023
Instruction: ./0023-IMPL-INSTR.md
Evidence-Mode: LOCAL

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `C01` | IMPLEMENTED | `Value.new` now collapses absent and empty `MultiAsset` inputs to the coin-only CBOR form while retaining the tuple for non-empty assets. | Exact-CBOR tests cover absent, empty, and non-empty assets, including `4_000_000n` as `1a003d0900`. |
| `C02` | IMPLEMENTED | Value arithmetic inherits the normalized construction boundary, and direct/output-builder amount construction emits a coin node whenever the semantic value has no assets. | Arithmetic, direct output, output-builder, and built transaction-body regressions pass. |
| `C03` | IMPLEMENTED | `from_cbor_*` parsing remains unchanged; received `[coin, {}]` values and transaction bodies retain their original bytes and transaction hashes until explicitly rebuilt. | Preserved value bytes, transaction-body bytes, and hash-sensitive reparse assertions pass. |
| `C04` | IMPLEMENTED | The chain README documents constructed versus decoded ADA-only behavior without new dependencies or package changes. | Documentation review, all 187 workspace tests, packed-package consumers, and `git diff --check` pass. |

## Outcome

New ADA-only values and outputs now use the ledger-compatible coin-only encoding. Existing received
CBOR remains lossless, so inspection and signing do not silently alter transaction bytes or hashes.

## Inputs consumed

- Human-provided Eternl diagnostic recorded in the instruction.
- `libs/typescript/packages/chain/src/era/conway/model.ts` and
  `libs/typescript/packages/chain/src/builder/transaction.ts`.
- Chain tests and README, plus accepted ADR 0001 for lossless CBOR and encoding metadata.

## Project changes

- Normalized empty assets in `Value.new` and constructed transaction-output amount nodes.
- Added exact-CBOR, arithmetic, output-builder, transaction-body, and hash-preservation regressions.
- Documented the distinction between constructed canonical values and losslessly decoded values.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `C01` | Constructing a value with absent or empty assets emits the same coin-only CBOR. | Public signatures and token-bearing values are unchanged. | Depend on coin-only CBOR for newly constructed ADA-only values. |
| `C02` | Arithmetic and output construction cannot reintroduce an empty multi-asset tuple for an ADA-only result. | Existing valid token-bearing output semantics remain unchanged. | No workaround is required in builders. |
| `C03` | Decoding remains byte-preserving even for received `[coin, {}]`. | Existing transaction bytes and hashes are not silently repaired. | Explicitly rebuild a received malformed transaction to canonicalize it. |
| `C04` | Package documentation and completion coverage define the construction/preservation boundary. | No dependency or package-boundary change. | Review the documented boundary before acceptance. |

## Validation

- `npm --prefix libs/typescript run build`: PASS.
- `node --test libs/typescript/packages/chain/test/conway-foundations.test.mjs libs/typescript/packages/chain/test/builders.test.mjs`: PASS; 24 focused tests.
- `npm --prefix libs/typescript run check`: PASS; 187 tests and packed ESM, NodeNext, and bundler consumers; 566 intended files.
- Exact `Value.new(4_000_000n, MultiAsset.new()).to_cbor_hex()`: PASS; `1a003d0900`.
- Empty-asset arithmetic and output-builder/body normalization: PASS.
- Non-empty asset tuple preservation: PASS.
- Received `[coin, {}]` value/body byte and transaction-hash preservation: PASS.
- `git diff --check`: PASS.

## Deviations from instruction

None.

## Remaining human review

Review the normalization boundary and confirm that explicit reconstruction, rather than decoding or
signing, is the intended point at which received empty-asset tuples become coin-only values.

## Reproducibility

Run `npm --prefix libs/typescript run check` and `git diff --check` from the repository root.
