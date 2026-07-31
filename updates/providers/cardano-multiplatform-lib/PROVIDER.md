# Cardano Multiplatform Lib provider

Provider: cardano-multiplatform-lib
Provider-Version: v1

## Purpose

Preserve the frozen CDDL comparison baseline and test-vector artifacts required by Cardano Lib's
compatibility tests. This provider is historical evidence, not a live update source or a runtime
code generator.

## Source

| Field | Value |
| --- | --- |
| Repository | `https://github.com/dcSpark/cardano-multiplatform-lib.git` |
| Exact commit | `39681e0d435a71f7c47a2601507ab16e691abb9e` |
| Git tree | `172d2a1d1b47968592ec408ea0411ee108ae47fe` |
| Revision policy | Exact; never follow a branch or newer commit |
| Source mode | Frozen |
| Submodules | Not part of the source |
| License | MIT |

The three CML license artifacts are `LICENSE`, `LICENSE-EMURGO`, and `LICENSE-IOHK`.

## CDDL and legal artifact selection

Preserve specification paths exactly:

| Upstream selection | Expected files | Snapshot destination |
| --- | ---: | --- |
| `specs/cip25.cddl` | 1 | `artifacts/specs/cip25.cddl` |
| the three selected files below `specs/cip36/` | 3 | `artifacts/specs/cip36/` |
| the selected CDDL files below `specs/conway/` | 10 | `artifacts/specs/conway/` |
| the selected CDDL files below `specs/multiera/` | 16 | `artifacts/specs/multiera/` |
| the selected CDDL files below `specs/multiera-byron/` | 8 | `artifacts/specs/multiera-byron/` |
| `LICENSE`, `LICENSE-EMURGO`, `LICENSE-IOHK` | 3 | `artifacts/legal/` |

The selected CDDL inventory is exactly:

```text
specs/cip25.cddl
specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/address.cddl
specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_crypto/mod.cddl
specs/cip36/lib.cddl
specs/conway/address.cddl
specs/conway/assets.cddl
specs/conway/auxdata.cddl
specs/conway/block.cddl
specs/conway/certs.cddl
specs/conway/crypto.cddl
specs/conway/governance.cddl
specs/conway/lib.cddl
specs/conway/plutus.cddl
specs/conway/transaction.cddl
specs/multiera-byron/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/byron.cddl
specs/multiera-byron/byron/block.cddl
specs/multiera-byron/byron/delegation.cddl
specs/multiera-byron/byron/mod.cddl
specs/multiera-byron/byron/mpc.cddl
specs/multiera-byron/byron/transaction.cddl
specs/multiera-byron/byron/update.cddl
specs/multiera-byron/lib.cddl
specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/address.cddl
specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/assets.cddl
specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/auxdata.cddl
specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/block.cddl
specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/byron.cddl
specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/certs.cddl
specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/crypto.cddl
specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/mod.cddl
specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/plutus.cddl
specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/transaction.cddl
specs/multiera/allegra/mod.cddl
specs/multiera/alonzo/mod.cddl
specs/multiera/babbage/mod.cddl
specs/multiera/lib.cddl
specs/multiera/mary/mod.cddl
specs/multiera/shelley/mod.cddl
```

Reject missing, additional, renamed, symlinked, or non-CDDL files within the selected scopes. The
reference-only `specs/byron.cddl`, `specs/byron_minimal.cddl`, `specs/shelley.cddl`, and
`specs/README.md` are excluded from the reviewed baseline.

## Test-vector artifact selection

Copy these regular files byte-for-byte from the exact CML commit, using this deterministic mapping:

| Upstream selection | Files | Snapshot destination |
| --- | ---: | --- |
| `chain/rust/src/genesis/byron/test_data/*.json` | 4 | `artifacts/test-vectors/genesis/byron/` |
| `chain/rust/src/genesis/shelley/test_data/{test.json,test-yaci.json}` | 2 | `artifacts/test-vectors/genesis/shelley/` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/*.cbor` | 34 | `artifacts/test-vectors/blocks/mainnet/` |
| `multi-era/rust/tests/golden_vectors/pallas/*.block` | 52 | `artifacts/test-vectors/blocks/pallas/` |
| `multi-era/rust/tests/golden_vectors/PROVENANCE.md` | 1 | `artifacts/test-vectors/PROVENANCE.md` |

Do not select additional files from either `test_data/` tree. Pallas `u5c*` vectors are excluded.
Preserve all 92 vector bytes and the upstream provenance document exactly; only the deterministic
directory mapping changes.

Create these snapshot-local control artifacts:

- `artifacts/test-vectors/manifest.json`: authoritative deterministic inventory of all 92 vectors,
  including logical path, tracked snapshot path, stored size, SHA-256, storage form, era metadata
  where applicable, and expected result.
- `artifacts/test-vectors/PROVENANCE.json`: CML source identity, manifest location, supplemental
  checksums, and the pinned Dolos/Pallas source and license mappings carried by CML.
- `artifacts/test-vectors/README.md`: corpus layout, path mapping, refresh policy, and consumer
  guidance.
- `artifacts/test-vectors/.gitattributes`: disable text conversion for block, CBOR, and JSON bytes.
- `artifacts/test-vectors/LICENSE-APACHE-2.0.txt`: Apache-2.0 text for the Dolos/Pallas vectors,
  with SHA-256 `4541e95aa81113643b71a96d7ff673c4a83ede3d2e8f0df2ad676e7970e2b1fa`.

These five files are deterministic snapshot metadata rather than byte-exact CML source. Package
tests must validate the manifest, provenance, licenses, and local Git attributes.

## Consumption and planning requirements

- Preserve the stable logical `specs/` prefix and the
  `_CDDL_CODEGEN_EXTERN_DEPS_DIR_` specification scopes.
- Preserve deterministic provenance, exact inventory, current owned-source behavior, and public
  API.
- Package tests consume vectors directly from `artifacts/test-vectors/`; do not duplicate them
  below packages, a root fixture directory, or another provider.
- Later snapshots may reuse this corpus only by its full immutable snapshot path and must name it
  under `Comparison sources` and in the artifact/change map.
- Replacing the frozen CML baseline requires an explicit new snapshot plan.
- A new snapshot at the same exact commit and provider version is a duplicate.

The `plutus.cddl` files describe ledger data and transaction wire grammar. They do not define or
authorize UPLC language or evaluator work.

## Excluded source material

- Reference-only CDDL outside the selected 38-file baseline
- Rust source, Rust test code, unrelated test data, build configuration, and Git metadata
- Pallas `u5c*` vectors
- Message-signing functionality
- UPLC language parsing, evaluation, Flat encoding, or builtin semantics
