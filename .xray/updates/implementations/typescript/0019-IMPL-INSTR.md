# TypeScript implementation 0019 instruction

Implementation-Version: v1
Implementation-ID: typescript/0019
Created: 20260803T104819Z
Evidence-Mode: LOCAL
Depends-On: ./0003-IMPL-RESULT.md, ./0004-IMPL-RESULT.md
Provider-Evidence: NONE

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| `typescript/0003` | `IMPLEMENTATION_RESULT` | Yes | Accepted UPLC Flat/program-envelope behavior |
| `typescript/0004` | `IMPLEMENTATION_RESULT` | Yes | Accepted Conway Script/ScriptRef wire owners |
| Chain script models and Plutus Flat codec | `LOCAL` | Yes | Existing nominal owners and strict decoders |
| Adjacent xray-js script helpers | `LOCAL` | Yes | Read-only tag-24 and heuristic double-CBOR workaround evidence |

## Objective

Add named typed ledger script factories and an explicit, strict Plutus serialized-script envelope
API for raw Flat, single-CBOR, and double-CBOR forms.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `TS19-SCRIPT1` | Add Native/Plutus V1/V2/V3 `Script` factories, kind, and accessors | Additive; generic raw construction remains | Chain Conway `Script` | All variants and malformed tags |
| `TS19-REF1` | Add typed `ScriptRef` factory/accessor for tag 24 | Additive | Chain Conway `ScriptRef` | Exact tag/payload, malformed tests |
| `TS19-ENV1` | Add strict explicit raw/single/double `SerializedPlutusScript` envelopes | Additive | Plutus package | Valid, malformed, ambiguous, mutation tests |
| `TS19-EXPORT1` | Preserve focused/root/aggregate binding identity by ownership | Additive exports | Chain, Plutus, runtime | Import/browser/packed tests |

## Implementation steps

1. Add named factories and typed accessors directly to existing chain `Script` and `ScriptRef`.
2. Add a Plutus-owned immutable envelope class and enum with explicit constructors for each form.
3. Validate raw Flat programs and exact CBOR byte-string nesting; do not auto-guess an envelope.
4. Expose explicit conversions among raw, single-CBOR, and double-CBOR owned bytes.
5. Add canonical, malformed, trailing-data, ambiguity, mutation, identity, browser, and packed tests.

## Validation

- Cover all four ledger script variants and tag-24 round trips.
- Cover raw Flat, single-CBOR, and double-CBOR exact bytes.
- Reject wrong CBOR kinds, wrong nesting, trailing bytes, invalid Flat programs, and extra wrapping.
- Prove defensive input/output ownership and public export identity.
- Run focused chain/Plutus/runtime tests and the full TypeScript gate.

## Compatibility and human review

Additive. Review which byte level the chain Plutus script owners contain, strict envelope parsing,
and the absence of heuristic auto-normalization.

## Completion criteria

Consumers can create ledger scripts and references without numeric discriminants or manual tag 24,
and can explicitly normalize serialized UPLC without double-CBOR guessing.

## Out of scope

- Script compilation, source languages, provider lookup, or policy evaluation

## Blockers

None.
