# C++ implementation 0004 result

Result-Version: v1
Implementation-ID: cpp/0004
Instruction: ./0004-IMPL-INSTR.md
Evidence-Mode: HYBRID

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `C4-HASH1` | Implemented | Added an owned 20-byte Blake2b-160 primitive beside the existing hash sizes. | Crypto and CIP-14 test units pass strict compilation. |
| `C4-CIP1` | Implemented | Added the nominal `AssetFingerprint` constructor/parser over existing policy and asset-name owners. | All eight captured vectors and round-trip cases are represented in tests. |
| `C4-ERR1` | Implemented | Added strict HRP, case, checksum, padding, length, policy, and asset-name rejection. | Negative and mutation test cases pass strict compilation. |
| `C4-API1` | Implemented | Exported one binding through focused, CIP umbrella, and aggregate headers. | Header/source/test units pass warnings-as-errors compilation. |

## Outcome

C++ consumers can construct and strictly parse CIP-14 asset fingerprints without duplicating policy or asset-name ownership.

## Inputs consumed

- `cpp/0001` accepted result
- `0001-cardano-cips` snapshot, captured CIP-14 text, and eight vectors
- Existing crypto, Bech32, `ScriptHash`, `AssetName`, packaging, and documentation owners

## Project changes

- Added Blake2b-160 and the CIP-14 focused module.
- Added captured-vector, strict parsing, and nominal-identity tests.
- Wired source/export/documentation ownership.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `C4-HASH1` | Blake2b-160 returns exactly 20 owned bytes. | Additive. | Use it where a 160-bit digest is required. |
| `C4-CIP1` | Fingerprints bind policy plus asset-name bytes to strict CIP-14 text. | Additive nominal type. | Reuse existing policy/name owners. |
| `C4-ERR1` | CIP-14 parsing is application-strict. | Generic Bech32 remains unchanged. | Handle rejected mislabeled or malformed text. |
| `C4-API1` | Every public path names the same type. | Additive exports. | No wrapper is needed. |

## Validation

- Fresh Apple Clang 21 Release and 50,000-case hardening configurations each passed all 150 tests; ASan/UBSan passed all 149 registered tests.
- CIP-14 vectors, formatting, API inventory, provider integrity, archive checks, and all six installed-component consumers passed.

## Deviations from instruction

The named presets were reproduced with equivalent fresh configurations using the existing pinned dependency installation because the reconstructed vcpkg checkout lacked host Autotools.

## Remaining human review

Confirm all captured vectors, installed export identity, and equivalent-workflow setup before acceptance.

## Reproducibility

Run the `ci`, `sanitizers`, and `hardening` workflows from `libs/cpp`.
