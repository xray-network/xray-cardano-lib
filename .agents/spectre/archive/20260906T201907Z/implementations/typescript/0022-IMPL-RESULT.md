# TypeScript implementation 0022 result

Result-Version: v1
Implementation-ID: typescript/0022
Instruction: ./0022-IMPL-INSTR.md
Evidence-Mode: LOCAL

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `C01` | Implemented | The CIP root now exposes every stable implementation as ordered `cip4`, `cip8`, `cip14`, `cip21`, `cip25`, `cip36`, `cip67`, and `cip68` namespaces; provisional CIP-129 remains focused-only. | Root/focused identity, browser-package, and packed-consumer tests pass. |
| `C02` | Implemented | The Plutus owner now exposes `data`, `uplc`, and `blueprint` namespaces while retaining its canonical owned exports. | Plutus API, serialized-script identity, and packed type consumers pass. |
| `C03` | Implemented | The aggregate package is namespace-only: `core`, `crypto`, `chain`, `cips`, and `plutus`; obsolete internal forwarding barrels and empty directories were removed. | Aggregate contract tests and packed ESM, NodeNext, and bundler consumers pass. |
| `C04` | Implemented | Updated CIP, Plutus, and aggregate documentation and expanded contract coverage for all stable namespaces. | Complete TypeScript gate and documentation/source scans pass. |

## Outcome

Cardano Lib now has one explicit owner per domain and a small namespace-only aggregate. Stable CIPs
and Plutus subdomains are discoverable without flattening thousands of unrelated symbols or
creating duplicate nominal owners.

## Project changes

- Normalized CIP and Plutus root barrels and their public documentation.
- Replaced the aggregate flat barrel with five domain namespaces.
- Removed obsolete aggregate forwarding source folders.
- Migrated aggregate, browser, downstream-pilot, and packed-consumer tests.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `C01` | Stable CIPs resolve through `cips.cipN`; CIP-129 is focused-only. | Breaking root cleanup. | Use the stable namespace or the explicit CIP-129 subpath. |
| `C02` | Plutus exposes `plutus.data`, `plutus.uplc`, and `plutus.blueprint`. | Additive owner structure. | Prefer the relevant Plutus subdomain. |
| `C03` | Aggregate symbols are grouped by domain. | Flat aggregate imports are removed. | Import `chain.Address`, `crypto.PrivateKey`, and equivalent owner namespaces. |
| `C04` | Tests and docs define only the new contract. | No compatibility aliases. | Migrate before publication. |

## Validation

- `npm --prefix libs/typescript run check`: PASS; 184 tests, packed ESM/NodeNext/bundler consumers, 566 intended files.
- Aggregate and stable-CIP identity checks: PASS; CIP-129 absent from stable roots.
- Empty aggregate forwarding directories removed.
- `git diff --check`: PASS.

## Deviations from instruction

None.

## Remaining human review

Review namespace naming, focused CIP-129 access, and the intentional removal of flat aggregate
compatibility before accepting the record.

## Reproducibility

Run `npm --prefix libs/typescript run check` from the repository root.
