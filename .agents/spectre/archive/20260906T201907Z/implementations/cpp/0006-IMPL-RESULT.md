# C++ implementation 0006 result

Result-Version: v1
Implementation-ID: cpp/0006
Instruction: ./0006-IMPL-INSTR.md
Evidence-Mode: HYBRID

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `C6-DOC1` | Implemented | Added bounded CIP-57 blueprint parsing/modeling for preamble, validators, definitions, parameters, code, and hashes. | Blueprint source and focused tests pass strict compilation. |
| `C6-SCH1` | Implemented | Added a private local-reference schema graph for Data/builtin forms and applicators with resource limits. | Reference, applicator, duplicate-key, and limit cases compile. |
| `C6-DAT1` | Implemented | Added read-only validation of existing `plutus::Data` and `UplcConstant` owners. | Data/builtin and recursive cases pass strict compilation. |
| `C6-COD1` | Implemented | Added serialized-code envelope validation and versioned Blake2b-224 hash verification. | Code/hash positive and malformed cases compile. |
| `C6-API1` | Implemented | Published one blueprint binding through Plutus and aggregate facades. | Public source/header/test units compile with warnings as errors. |

## Outcome

C++ consumers gain a bounded, offline CIP-57 blueprint API that validates existing Plutus values and compiled scripts without a generic resolver or duplicate Data type.

## Inputs consumed

- `cpp/0001` accepted result
- `0001-cardano-cips` snapshot, captured CIP-57 document, and five schema files
- Existing C++ Plutus Data, UPLC, hashing, JSON, package, and documentation owners

## Project changes

- Added the blueprint public model/parser and private schema evaluator.
- Added duplicate-key, local-reference, value, code/hash, and resource-limit tests.
- Wired focused, component, and aggregate exports.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `C6-DOC1` | Blueprint documents are admitted through a typed bounded parser. | Additive. | Parse untrusted blueprints before use. |
| `C6-SCH1` | Only local/captured schema identities resolve. | No generic JSON Schema API. | Do not supply external resolvers. |
| `C6-DAT1` | Existing Data/constants are validated without coercion. | Existing owners unchanged. | Pass the established Plutus bindings. |
| `C6-COD1` | Compiled code must match its declared versioned hash. | Raw UPLC decode remains available. | Treat blueprint admission as the stricter boundary. |
| `C6-API1` | Focused and aggregate imports share one owner. | Additive exports. | No blueprint adapter is needed. |

## Validation

- Fresh Apple Clang 21 Release and 50,000-case hardening configurations each passed all 150 tests; ASan/UBSan passed all 149 registered tests.
- Blueprint parsing/value tests, formatting, inventory, provider integrity, installed content, and all six component consumers passed.

## Deviations from instruction

The named presets were reproduced with equivalent fresh configurations using the existing pinned dependency installation because the reconstructed vcpkg checkout lacked host Autotools. No external resolver or provider runtime dependency was introduced.

## Remaining human review

Review schema-vocabulary fidelity, resource ceilings, code/hash admission, and equivalent-workflow setup before acceptance.

## Reproducibility

Run the documented C++ completion/security workflows and blueprint focused tests from `libs/cpp`.
