# TypeScript implementation 0003 result

Result-Version: v1
Implementation-ID: typescript/0003
Instruction: ./0003-IMPL-INSTR.md
Evidence-Mode: DIRECT

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `C001` | `IMPLEMENTED` | `libs/typescript/packages/plutus/` | `npm --prefix libs/typescript run check` |

## Outcome

Implemented the official Plutus UPLC runtime and raw phase-two transaction valuation as
browser-native, package-owned TypeScript. `@xray-network/cardano-plutus` exposes typed Data,
immutable UPLC program/term/constant/data values, text parsing, Flat and serialized-script codecs,
cost-model construction, default machine costs, budgeted CEK evaluation,
`apply_params_to_script`, and `eval_phase_two_raw`. The focused `./data` and `./uplc` subpaths and
the root runtime facade re-export the same identities.

The implementation covers UPLC 1.0.0 and 1.1.0, Flat terms and constants, builtin tags 0-100,
semantics variants A-E, exact current cost-model formulas, restricting CEK budgets, Trace logs,
protocol/language availability, and iterative handling of adversarial term depth. Raw valuation
supports protocol majors 5-11, Plutus V1/V2/V3, Alonzo/Babbage/Conway contexts, all six redeemer
purpose tags, witness and reference scripts, datum resolution, optional collection checks,
independent per-redeemer maximum budgets, and memory/CPU rewriting into ledger ExUnits order.

Parameter application is strict, complete-input, left-to-right, scope-checked, canonical, bounded
to 16 MiB, and defensively copies caller data. Phase-two decoding retains only the official V1/V2
CBOR-remainder exception, keeps V1's historical Data-leaf behavior, enforces the V2+ 64-byte
on-wire rule, and applies protocol-11 universe and constructor bounds.

## Artifacts consumed

- `../../providers/uplc/0001-uplc/artifacts/SHA256SUMS`
- All 24 byte-exact files under `../../providers/uplc/0001-uplc/artifacts/cardano-ledger/`
- `../../providers/uplc/0001-uplc/artifacts/conformance/README.md`
- `../../providers/uplc/0001-uplc/artifacts/conformance/corpus.json`
- All 113 byte-exact files under `../../providers/uplc/0001-uplc/artifacts/plutus/`, including the Plutus legal/provenance files,
  conformance runner evidence, 68 selected Plutus Core files, and 38 selected Plutus Ledger API
  files

The authoritative checksum inventory passed for all 139 listed artifacts. The conformance reader
also verified the size and SHA-256 digest of every one of the 3,013 embedded corpus entries before
executing 1,003 UPLC programs. The captured source identities remain official Plutus release
`1.66.0.0` at commit `91e8c2af9c7bec705b035c6cf8e679c35c4c2ad5` and Cardano Ledger commit
`a624de4c8db7286a6c065da149679ea55f7d5629`, under the captured Apache-2.0 materials.

## Project changes

- Added the public UPLC AST, iterative Flat codec, text parser, exact A-E cost-model data/formulas,
  CEK evaluator, memory accounting, and official builtin denotations under
  `libs/typescript/packages/plutus/src/uplc/`.
- Added private ledger decoding, script/datum/UTxO resolution, phase-one collection checks,
  era-sensitive V1/V2/V3 context translation, and raw evaluation under
  `libs/typescript/packages/plutus/src/ledger/`.
- Added the typed Data API, UPLC API, raw-evaluation functions, result contracts, Plutus package
  exports, and identity-preserving root runtime exports.
- Extended the Cardano-owned cryptography boundary with SHA2-256, Keccak-256, RIPEMD-160,
  secp256k1 ECDSA/Schnorr, and BLS12-381 primitives under ADR 0004. Existing exact noble `2.2.0`
  pins remain unchanged.
- Added stable `EVALUATE` and `UNSUPPORTED` Cardano error codes for deterministic valuation and
  availability failures.
- Added Plutus tests under `libs/typescript/packages/plutus/test/` and configured root test discovery for the
  package layout.
- Added focused API, Flat, cost-model, machine, ledger/context, crypto, full-corpus, browser,
  identity, malformed-input, deep-term, and builder valuation-to-`set_exunits` coverage.
- Added no dependency, WASM, native addon, Node-only production edge, or upstream runtime artifact.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `C001` | Add the official UPLC 1.0.0/1.1.0 runtime, Flat codec, CEK evaluation, builtins 0–100, cost models, Plutus V1/V2/V3 contexts, parameter application, and raw phase-two valuation for protocol majors 5–11. | Compatible additive API with consensus-sensitive evaluation semantics. | Implement the supported semantic contract from this result, use the captured provider evidence directly, combine both sources, or record the change as not applicable. |

## Validation

| Check | Result | Evidence |
| --- | --- | --- |
| `shasum -a 256 -c SHA256SUMS` from `../../providers/uplc/0001-uplc/artifacts/` | PASS | All 139 captured artifacts matched |
| `npm --prefix libs/typescript run build` | PASS | All TypeScript project references built |
| `node --test libs/typescript/packages/crypto/test/*.test.mjs` | PASS | Existing and UPLC crypto vectors, malformed inputs, and dependency policy passed |
| `node --test libs/typescript/packages/plutus/test/*.test.mjs` | PASS | API, Data, Flat, cost, CEK, ledger, and all 1,003 applicable official programs passed |
| runtime import/API/browser tests | PASS | Function identities, universal ESM shape, and browser graph passed |
| `npm --prefix libs/typescript run test:built` | PASS | 137 workspace tests passed |
| `node libs/typescript/packages/runtime/test/pack-smoke.mjs` | PASS | 526 intended files; ESM, NodeNext, and bundler consumers passed |
| `npm --prefix libs/typescript run check` | PASS | Authoritative build, 137-test workspace suite, and packed-consumer gate passed |
| `git diff --check` | PASS | No whitespace errors |

## Deviations from plan

None.

## Remaining human review

- Review consensus-sensitive builtin error order, cost accounting, and protocol/language
  availability despite the complete captured conformance corpus passing.
- Review Alonzo, Babbage, and Conway context translation and the intentionally narrow V1/V2
  serialized-script remainder exception.
- Review the new cryptographic acceptance rules, especially signature strictness, BLS subgroup and
  infinity handling, domain separation, and compressed encodings. Automated evidence is not an
  independent security audit; ADR 0002's stable-release review requirement remains.
- Review the public API and package-size impact of the package-owned exact cost-model tables.

## Reproducibility

Implementation used only the captured, checksummed snapshot artifacts. It did not fetch, refresh,
execute, or substitute upstream material. Published packages contain only reviewed Cardano Lib
TypeScript output and do not load snapshot artifacts at runtime.
