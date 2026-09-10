# C++ implementation 0008 result

Result-Version: v1
Implementation-ID: cpp/0008
Instruction: ./0008-IMPL-INSTR.md
Evidence-Mode: HYBRID

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `C8-DIA1` | Implemented | Added bounded, stable, sorted CIP-21 diagnostics over an explicit era and complete serialized transaction. | Diagnostic source and focused tests pass strict compilation. |
| `C8-CBR1` | Implemented | Inspected preserved CBOR for canonical widths, definite lengths, map order, and tag-258 consistency. | Raw encoding mutation cases compile with warnings as errors. |
| `C8-BDY1` | Implemented | Added captured body/output/multiasset/certificate/withdrawal/voting restrictions and count/range limits. | Rule and interaction cases pass strict compilation. |
| `C8-AUX1` | Implemented | Added opt-in Catalyst tuple diagnostics while leaving hash-only mode content-agnostic. | Both mode paths compile. |
| `C8-API1` | Implemented | Published the optional diagnostic through focused/component/aggregate facades without builder enforcement. | Export/source/test units pass strict compilation. |

## Outcome

C++ applications can request deterministic, non-authoritative CIP-21 compatibility findings without mutating or canonicalizing transaction bytes.

## Inputs consumed

- `cpp/0001` accepted result
- `0001-cardano-cips` snapshot and captured active CIP-21 document
- Existing preserved CBOR, era transaction, CIP-36, error, package, and documentation owners

## Project changes

- Added the CIP-21 public diagnostic model and preserved-CBOR inspector.
- Added encoding, body, output, governance, Catalyst, ordering, and cap tests.
- Wired optional exports and documented the advisory device boundary.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `C8-DIA1` | Diagnostics are stable, ordered, bounded, and era-explicit. | Additive read-only API. | Treat findings as compatibility advice. |
| `C8-CBR1` | Findings reflect preserved wire encoding. | No canonicalization side effect. | Retain source bytes when diagnosing. |
| `C8-BDY1` | Captured CIP-21 restrictions are reported independently of ledger validity. | Valid ledger transactions may still receive findings. | Decide device policy at the caller. |
| `C8-AUX1` | Catalyst structure is checked only in the selected mode. | Hash-only remains content-agnostic. | Opt in only for Catalyst registration. |
| `C8-API1` | Diagnostics remain optional and do not affect building. | Additive exports. | Call explicitly before device submission. |

## Validation

- Fresh Apple Clang 21 Release and 50,000-case hardening configurations each passed all 150 tests; ASan/UBSan passed all 149 registered tests.
- CIP-21 tests, formatting, inventory, provider integrity, installed content, and all six component consumers passed.

## Deviations from instruction

The named presets were reproduced with equivalent fresh configurations using the existing pinned dependency installation because the reconstructed vcpkg checkout lacked host Autotools.

## Remaining human review

Review preserved-CBOR coverage, every stable code/path, Catalyst mode isolation, the diagnostic cap, and equivalent-workflow setup before acceptance.

## Reproducibility

Run the complete C++ workflows and focused CIP-21 tests from `libs/cpp`.
