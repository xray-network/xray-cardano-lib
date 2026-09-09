# cardano-multiplatform-lib capture 0002 summary

Provider-Capture-Version: v1
Provider: cardano-multiplatform-lib
Capture: 0002
Snapshot: SNAPSHOT.md
Previous-Capture: 0001/SNAPSHOT.md#change-summary

## Summary

This summary retains the original upstream review findings. The human-requested development
reconciliation changes storage and metadata only, not upstream revisions or runtime behavior.
SNAPSHOT.md is the sole authority for specification, inventories, counts and removals.

## Change summary

The upstream update adds bounded CIP-36/builder evidence while preserving the vector corpus.
The authoritative artifact comparison, including reconciled control-file paths, is in
[SNAPSHOT.md](SNAPSHOT.md#changes-from-predecessor).

| Change | Previous/current evidence | Observed significance |
| --- | --- | --- |
| CIP-36 metadata view | [old schema](../0001/artifacts/specs/cip36/lib.cddl), [new schema](artifacts/specs/cip36/lib.cddl), [utils](artifacts/upstream/cip36/rust/src/utils.rs) | Registration/deregistration schemas now retain arbitrary metadata labels. Captured behavior includes preserved full-map conversions and rest-label merges. |
| CDDL comments/codegen annotations | [Plutus grammar](artifacts/specs/conway/plutus.cddl), [Byron transaction grammar](artifacts/specs/multiera-byron/byron/transaction.cddl) | Diff changes comments/directives, not wire alternatives. No wire-code update inferred. |
| Explicit selector change | [source inventory](SNAPSHOT.md#source-selection-and-mapping); [old inventory](../0001/SNAPSHOT.md#verified-artifact-inventory) | Ten multi-era extern paths removed; two CIP36 dependencies and twelve bounded Rust files newly selected. New Rust evidence has no captured 0001 Rust baseline. |
| Corpus and legal evidence | [vector manifest](artifacts/test-vectors/manifest.json), [provenance](../0001/artifacts/test-vectors/PROVENANCE.md), [licenses](../0001/artifacts/legal/LICENSE) | The complete vector set, upstream provenance and three licenses are unchanged. Vector bytes are reused; the rebased manifest, current provenance and checksum controls resolve earlier files. |

## Consumer impact and recommended work

Paths below are repository-root-relative, describing maintained TypeScript at capture time. Evidence
semantics are language-neutral; C++ is unmaintained and opt-in. Recommendations do not change source
or authorize plans. The current seven LOCAL implementation records remain REVIEW.

| Finding | Maintained modules/APIs | Tests and validation | Recommendation and evidence |
| --- | --- | --- | --- |
| Positive weight predicate | libs/typescript/packages/cip/src/cip36/metadata.ts, CIP36RegistrationCbor.verify | libs/typescript/packages/cip/test/cip36.test.mjs | Already aligned by local 0025: at least one positive uint32 weight. Keep boundary/mixed-zero tests; utils.rs and invariants.rs support this predicate. |
| Decode-time invariant gap | Same module, from_metadata_bytes/try_from_metadata | Same test file; add invalid all-zero metadata decoding cases | Investigate/plan: upstream generated/serialization.rs calls extra_validation on deserialize and invariants.rs explicitly rejects all-zero metadata. TS currently validates on verify/output/merge, not view creation. Decide compatibility before tightening decode. |
| Full metadata preservation | Same module, registration/deregistration to_metadata_bytes and try_into_metadata | Existing preserved/canonical and defensive-buffer cases | Local 0026 aligns byte-preserving full-map conversion with the schema rest rows. Keep those regressions; this is not proof of all upstream JSON behavior. |
| Merge semantic difference | Same module, add_to_metadata | Existing test intentionally preserves caller labels and does not import unrelated view labels | Explicit compatibility decision required: utils.rs copies rest labels into the destination; current TS intentionally merges only CIP36 labels. Do not silently alter the established TS contract. |
| JSON and unknown-field coverage | Same module, from_json/to_js_value; generated/mod.rs and tests/json.rs, unknown_keys.rs | Focused JSON/unknown-key tests | Investigate complete mappings before claiming parity: TS metadata views currently export only owned CIP36 fields; new Rust evidence must be reviewed field-by-field. |
| Strict construction sets and redeemer positions | libs/typescript/packages/chain/src/builder/transaction.ts | libs/typescript/packages/chain/test/builders.test.mjs | Local 0027 rejects duplicate certificates/proposals atomically and preserves full-sequence indexes. Upstream tx_builder.rs validates sets during body construction, ordered_set.rs distinguishes strict push/conversion from union insert, and redeemer_builder.rs retains empty placeholders. Keep the TS earlier-error policy explicit; broader optional sets/witness unions need separate comparison. |
| Wire grammar and vectors | libs/typescript/packages/chain/src/ and test/ | Historical round-trip/negative fixtures | No new wire-rule or vector-byte change identified in this selected comparison. Do not rewrite historical decoder behavior from construction-only constraints. |

## Semantic evidence

The comparison directly inspected every changed common CDDL file and the relevant CIP36 utilities,
serialization invariant hooks, upstream invariant tests, strict/union collection operations and
transaction/redeemer builder sections. The twelve Rust files are new capture coverage, so behavior
observations are a baseline for those files, not fabricated old-to-new Rust deltas.

## Unresolved questions

- Decide whether to align decode-time CIP36 validation while preserving intentionally lossless APIs.
- Decide the public merge contract for unrelated metadata labels; current TS and CML differ.
- Complete JSON/unknown-key and broader set/witness compatibility mapping before any parity claim.
- Extern CDDL references still require owned semantic mappings; capture is complete but does not
  establish a standalone grammar or full runtime conformance.

## Exclusions

Unselected source, reference-only CDDL, arbitrary Rust API parity, Pallas u5c* vectors, upstream
execution, message signing, UPLC evaluator semantics, schedules and automatic implementation.

