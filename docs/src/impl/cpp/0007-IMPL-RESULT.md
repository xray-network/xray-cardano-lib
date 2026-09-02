# C++ implementation 0007 result

Result-Version: v1
Implementation-ID: cpp/0007
Instruction: ./0007-IMPL-INSTR.md
Evidence-Mode: HYBRID

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `C7-LBL1` | Implemented | Added exact four-byte CIP-67 label codecs and labeled asset-name helpers under the experimental namespace. | Vector and strict-prefix cases pass strict compilation. |
| `C7-REL1` | Implemented | Added CIP-68 class/relationship construction over existing policy and asset owners. | Policy/content/class cases compile with warnings as errors. |
| `C7-DAT1` | Implemented | Added strict three-field CIP-68 datums for the required class/version matrix using chain-owned Plutus Data. | Datum round-trip and malformed cases pass strict compilation. |
| `C7-MET1` | Implemented | Added direct/v4-nested metadata, known-field, duplicate, URI/chunk, and resource-limit validation. | Metadata and URI test cases pass strict compilation. |
| `C7-API1` | Implemented | Kept CIP-67 focused/experimental and exported active CIP-68 through stable facades. | Export boundaries and source ownership compile. |

## Outcome

CIP-67 labels and CIP-68 relationships/datums are available with proposal-aware exports, existing nominal owners, strict metadata validation, and no network behavior.

## Inputs consumed

- `cpp/0001` accepted result
- `0001-cardano-cips` snapshot and captured CIP-67 registry/schema and CIP-68 specification
- Existing C++ `AssetName`, `ScriptHash`, Plutus Data, CIP, package, and documentation owners

## Project changes

- Added focused CIP-67 and stable CIP-68 modules.
- Added exact codec, relationship, datum, metadata, URI, and malformed-input tests.
- Wired build/export/documentation ownership while preserving proposal isolation.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `C7-LBL1` | Labels encode to exact bracket/CRC prefixes. | Additive provisional API. | Import the focused experimental header. |
| `C7-REL1` | Reference/user assets must share policy and content. | Additive pure validation. | Perform ledger lookup separately. |
| `C7-DAT1` | CIP-68 datums have exactly metadata/version/extra. | Additive strict parser. | Reject stale two-field examples. |
| `C7-MET1` | Metadata versions/classes and URI forms are checked offline. | Additional properties remain Data. | Supply the exact v4 policy/content path. |
| `C7-API1` | Stable CIP-68 and provisional CIP-67 exports stay separate. | Additive. | Do not rely on CIP-67 aggregate exposure. |

## Validation

- Fresh Apple Clang 21 Release and 50,000-case hardening configurations each passed all 150 tests; ASan/UBSan passed all 149 registered tests.
- CIP-67/68 tests, formatting, inventory, provider integrity, installed content, and all six component consumers passed.

## Deviations from instruction

The named presets were reproduced with equivalent fresh configurations using the existing pinned dependency installation because the reconstructed vcpkg checkout lacked host Autotools.

## Remaining human review

Review the version/class matrix, v4 nesting, URI/resource boundaries, export isolation, and equivalent-workflow setup before acceptance.

## Reproducibility

Run the complete C++ workflows and focused CIP tests from `libs/cpp`.
