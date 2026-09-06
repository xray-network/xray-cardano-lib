# C++ implementation 0005 result

Result-Version: v1
Implementation-ID: cpp/0005
Instruction: ./0005-IMPL-INSTR.md
Evidence-Mode: HYBRID

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `C5-KEY1` | Implemented | Extended move-only private-key forms and added role/form-aware CIP-5/CIP-16 key text codecs. | Crypto identity cases pass strict source/test compilation. |
| `C5-HD1` | Implemented | Added checked CIP-1852 paths and roles 0 through 5 over existing private/public derivation owners. | Boundary and role test cases pass strict compilation. |
| `C5-ADR1` | Implemented | Made domain Bech32 parsing enforce CIP-19 class/network HRPs and retained an explicit compatibility parser. | Address implementation compiles with warnings as errors. |
| `C5-GOV1` | Implemented | Added focused provisional CIP-129 credential/action identifiers and decode-only CIP-105 legacy identifiers. | Header, role/kind, length, and one-octet action-index tests pass strict compilation. |
| `C5-API1` | Implemented | Exported stable identity/derivation owners while keeping CIP-129 out of stable umbrellas. | Focused/aggregate source boundaries pass strict compilation. |

## Outcome

Keys, derivation paths, addresses, and provisional governance identifiers now have explicit role, form, network, and status boundaries.

## Inputs consumed

- `cpp/0001` accepted result
- `0001-cardano-cips` snapshot and captured CIP-5, CIP-16, CIP-19, CIP-105, CIP-129, and CIP-1852 material
- Existing C++ key, BIP32, address, Bech32, hash, package, and documentation owners

## Project changes

- Added identity and derivation modules and extended private-key representation.
- Hardened Shelley/reward address text parsing.
- Added isolated experimental CIP-129 APIs and tests.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `C5-KEY1` | Key text decoding requires the expected role and form. | Existing generic key methods remain. | Select role/form explicitly. |
| `C5-HD1` | CIP-1852 hardening and roles are encoded by checked paths. | Raw derivation remains. | Prefer typed paths for Cardano identities. |
| `C5-ADR1` | Address HRPs must agree with payload class/network. | Valid canonical addresses are unchanged. | Use the compatibility parser only for legacy ingestion. |
| `C5-GOV1` | CIP-129 stays provisional and action indexes are limited to one octet. | Additive focused API. | Import the focused experimental header explicitly. |
| `C5-API1` | Stable and provisional exports remain separated. | Additive except stricter mislabeled-address rejection. | Do not assume CIP-129 aggregate availability. |

## Validation

- Fresh Apple Clang 21 Release and 50,000-case hardening configurations each passed all 150 tests; ASan/UBSan passed all 149 registered tests.
- Identity/derivation/address/CIP-129 tests, formatting, inventory, provider integrity, installed content, and all six component consumers passed.

## Deviations from instruction

The named presets were reproduced with equivalent fresh configurations using the existing pinned dependency installation because the reconstructed vcpkg checkout lacked host Autotools.

## Remaining human review

Confirm secret-buffer handling, captured vector coverage, provisional isolation, and equivalent-workflow setup before acceptance.

## Reproducibility

Run all C++ completion and security presets plus installed-consumer checks from `libs/cpp`.
