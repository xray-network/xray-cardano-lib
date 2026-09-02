# TypeScript implementation 0024 result

Result-Version: v1
Implementation-ID: typescript/0024
Instruction: ./0024-IMPL-INSTR.md
Evidence-Mode: LOCAL

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `C01` | `IMPLEMENTED` | Completed exact-bigint body access through chain-owned collections, including typed withdrawals, signers, voting/proposals, protocol updates, and every supported body field. | Focused multi-era coverage and the complete TypeScript gate pass. |
| `C02` | `IMPLEMENTED` | Completed reference-only inputs and full Byron/Shelley-family output access for values, datum hashes/options, inline data, reference scripts, and collateral return. | Existing output/value suites plus focused inspection tests pass. |
| `C03` | `IMPLEMENTED` | Added historical tag `5/6` validation and semantic access, complete pool parameters, non-throwing raw/known certificate tags, and opaque future-tag retention. | Conway and multi-era certificate coverage passes for current, historical, malformed, and future tags. |
| `C04` | `IMPLEMENTED` | Added defensive typed witness and auxiliary-data component access for key/bootstrap witnesses, native/Plutus scripts, data, redeemers, and metadata. | Witness, auxiliary, builder, browser, and package tests pass. |
| `C05` | `IMPLEMENTED` | Added owned unknown-field records, allowed valid future fields at lossless map boundaries, and made standalone body era identity explicitly unknown while block-derived identity remains authoritative. | Unknown-field, standalone-era, block-vector, preservation, canonical, and hash tests pass. |
| `C06` | `IMPLEMENTED` | Exported the new owners through existing focused/root/aggregate identities, updated package documentation and mirrors, and extended focused tests. | Build, 188 tests, browser/package smoke, packed consumers, and diff checks pass. |

## Outcome

Cardano Lib now exposes complete intrinsic transaction inspection primitives without UTxO resolution,
application DTOs, signedness inference, or JSON inspection conversion. Exact decoded bytes remain the
preserved serialization source, while canonical encoding remains explicit.

## Inputs consumed

- The human request dated 2026-08-25 recorded by the instruction.
- The multi-era, Conway, Shelley, witness, auxiliary-data, and shared lossless codec owners under
  `libs/typescript/packages/chain/src/`.
- The focused chain tests named by the instruction and adjacent read-only xray-js/XRAY App consumers.
- Accepted lossless-CBOR/package-ownership ADRs and chain/runtime documentation.

## Project changes

- Extended Conway validation and typed model accessors for historical/future certificates, witnesses,
  auxiliary data, output data/scripts, pool parameters, signers, withdrawals, and unknown fields.
- Extended multi-era transaction bodies, certificates, outputs, updates, and protocol values with
  exact typed access and unknown standalone-era semantics.
- Added deterministic regression coverage and documented the intrinsic-only boundary.
- Synchronized canonical implementation records, Mintlify mirrors, status, and navigation.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `C01` | Every supported intrinsic body field has exact typed access. | Unreleased generic returns were replaced by chain-owned types. | Consume typed owners and `bigint` values. |
| `C02` | Inputs are references; outputs include all intrinsic value/data/script detail. | No resolution fields exist. | Resolve inputs separately only when an application explicitly needs that data. |
| `C03` | Certificate tags `0..18` are known and future tags retain CBOR. | Historical amount semantics remain distinct. | Handle `known_kind()` absence for future certificates. |
| `C04` | Witness and auxiliary components are observable facts. | Presence does not mean signing completeness. | Do not label witness presence as signed or valid. |
| `C05` | Unknown fields are lossless and standalone bodies have no fabricated era. | `kind()` may be absent for standalone bodies. | Use block context when authoritative era identity is required. |
| `C06` | Focused, root, and aggregate exports share nominal owners. | No new package or dependency. | Import through any existing intended path. |

## Validation

- `npm --prefix libs/typescript run build` — passed.
- Focused `node --test libs/typescript/packages/chain/test/multi-era.test.mjs` — 5 passed.
- `npm --prefix libs/typescript run check` — passed: 188 tests plus packed ESM, NodeNext, bundler, and browser/package smoke checks.
- `git diff --check` — passed.

## Deviations from instruction

None.

## Remaining human review

Review the public accessor names, future-field boundary, standalone-era semantics, historical certificate
payloads, and preserved-versus-canonical behavior.

## Reproducibility

Run `npm --prefix libs/typescript run check` from the repository root.
