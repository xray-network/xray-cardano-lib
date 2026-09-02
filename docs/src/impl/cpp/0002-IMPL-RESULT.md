# C++ implementation 0002 result

Result-Version: v1
Implementation-ID: cpp/0002
Instruction: ./0002-IMPL-INSTR.md
Evidence-Mode: HYBRID

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `C2-BLD1` | Implemented | Made spending, reference, and collateral identities pairwise disjoint and restored observable builder state after failed selection or construction. | Strict compilation covers the builder and rollback tests. |
| `C2-COL1` | Implemented | Added checked collateral percentage/count/total/return accounting with native-asset preservation. | Boundary, overlap, return, and asset cases were added to builder tests. |
| `C2-FEE1` | Implemented | Replaced the fixed eight passes with exact fee/change fixed-point detection, cycle rejection, a 32-pass cap, and rollback. | Convergence and failure-atomic paths compile with warnings as errors. |
| `C2-CFG1` | Implemented | Added exact nonnegative rational reference-script fees and a checked Conway protocol-parameter adapter. | Rational and adapter source/test translation units pass strict compilation. |
| `C2-CWY1` | Implemented | Closed raw constructors and added typed Conway transaction-body, proposal, voting, and protocol-update builders returning existing model owners. | Direct typed-construction and malformed-shape tests pass strict compilation. |
| `C2-API1` | Implemented | Published the additions through the existing chain and aggregate owners and documented the construction/ingestion boundary. | Header/source/test translation units pass strict compilation. |

## Outcome

Checked construction is atomic, collision-safe, collateral-complete, and convergence-bounded, and typed Conway construction no longer requires public unchecked CBOR constructors.

## Inputs consumed

- `cpp/0001` accepted result
- `0001-cardano-ledger` snapshot and its declared Conway artifacts
- Existing C++ builder, ledger, Conway model, test, package, and documentation owners

## Project changes

- Hardened builder mutation, collateral, fee, and configuration behavior.
- Added typed Conway construction and focused regression tests.
- Updated public API documentation and build ownership.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `C2-BLD1` | Input roles are disjoint and failed mutations are atomic. | Previously ambiguous invalid builders now reject. | Treat role overlap as a construction error. |
| `C2-COL1` | Collateral arithmetic and native assets are checked exactly. | Valid existing collateral remains valid. | Supply an exact return when collateral carries assets. |
| `C2-FEE1` | Builds succeed only at an exact fee/change fixed point. | Stable canonical results are retained. | Handle deterministic convergence failures. |
| `C2-CFG1` | Reference-script fees use exact rational arithmetic. | Integer convenience remains denominator-one. | Prefer the rational/config adapter APIs. |
| `C2-CWY1` | Typed Conway builders return the established nominal owners. | Additive, except unchecked raw construction is closed. | Use `from_value` for validated ingestion or typed builders for construction. |
| `C2-API1` | Focused and aggregate paths share the same bindings. | Additive exports. | No adapter or duplicate model is needed. |

## Validation

- Fresh Apple Clang 21 C++23 Release configuration/build passed all 150 tests, including format, inventory, provider-integrity, six installed-component consumers, and aggregate smoke.
- Fresh ASan/UBSan configuration/build passed all 149 registered tests.
- Fresh 50,000-case hardening configuration/build passed all 150 tests.

## Deviations from instruction

The named presets could not reuse the reconstructed vcpkg checkout without rebuilding missing host Autotools. Equivalent fresh configurations used the existing pinned dependency installation with manifest installation disabled; all configured build/test semantics passed.

## Remaining human review

Review the checked-builder policy, typed Conway canonical encodings, and equivalent-workflow setup before acceptance.

## Reproducibility

From `libs/cpp`, run `cmake --workflow --preset ci`, `cmake --workflow --preset sanitizers`, and `cmake --workflow --preset hardening` with the documented toolchain.
