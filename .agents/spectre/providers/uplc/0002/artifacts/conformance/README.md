# Official Plutus conformance corpus

This is an inert, deterministic capture of every regular source blob below
`plutus-conformance/test-cases/` at official Plutus tag 1.68.0.0, commit
`9e17e2404dc6988c908b1fea099dde202df73b6a`. It is evidence for a separately authorized consumer plan.

## Frozen inventory and transport

- schemaVersion 1; object key order: schemaVersion, source, entries.
- source key order: repository, commit, tag, root, licensePath.
- entry key order: path, size, sha256, contentBase64.
- UTF-8 JSON, two-space indentation, LF trailing newline; entries sorted by UTF-8 path bytes.
- Exactly 4,822 entries preserving 1,162,961 source bytes, maximum entry size
  19,938 bytes; per-entry capture limit 16 MiB.
- Canonical RFC 4648 padded base64 preserves binary bytes. Verify length, SHA-256, unique/safe
  paths and exact membership before using any data. Never interpret arbitrary entries as UTF-8.

## Case discovery and associations

Every .uplc or .flat entry is a distinct format case: 1,004 text and 901 Flat, totaling 1,905.
Each has `<program path>.expected`; this capture uses `<program path without extension>.budget.expected`.
All 1,004 budget files are shared by same-stem text/Flat cases where both exist. For historical
captures only, consumers may additionally support `<program path>.budget.expected`, but must
reject missing or ambiguous associations. All result and budget files in this capture are accounted for.
There are 103 text-only cases; never invent Flat counterparts or silently omit them.

Expected results: text has 721 successes, 219 evaluation failures and 64 parse/decode failures;
Flat has 677 successes, 219 evaluation failures and 5 parse/decode failures. These are fixture
expectations, not a report that the current TypeScript runtime executed or passed these cases.
A successful Flat expected result is binary; only explicit short ASCII failure markers are text.
Budget success uses the captured CPU/memory record; failure markers distinguish decoding/evaluation.
The captured upstream README and Common.hs define the release’s runner/format conventions.

## Auxiliary entries

These eight entries are support evidence, not executable cases. All remain in corpus.json and
must be explicitly accounted for by consumers; the shell script is never executed or installed.

- `uplc/evaluation/builtin/interleaving/README.md`
- `uplc/evaluation/builtin/parser/README.md`
- `uplc/evaluation/builtin/semantics/README.md`
- `uplc/evaluation/builtin/semantics/bls12_381-cardano-crypto-tests/README.md`
- `uplc/evaluation/builtin/semantics/bls12_381_G1_multiScalarMul/files`
- `uplc/evaluation/builtin/semantics/ren.sh`
- `uplc/evaluation/builtin/semantics/verifyEcdsaSecp256k1Signature/README.md`
- `uplc/evaluation/term/parser/README.md`

The six Markdown files provide prose; `files` is a textual case-development note and `ren.sh` is
upstream rename tooling. Its Git executable mode is provenance only: its bytes are base64 data
inside corpus.json, not executable repository tooling. No upstream tooling ran during capture.

## License and consumption

The byte-exact Apache-2.0 license and notice are at
[LICENSE](../../../0001/artifacts/plutus/plutus-conformance/LICENSE) and
[NOTICE](../../../0001/artifacts/plutus/plutus-conformance/NOTICE).
Verify [SHA256SUMS](../SHA256SUMS) from the provider root using its provider-root-relative paths.
[SNAPSHOT.md](../../SNAPSHOT.md) owns the complete resolved inventory and capture rules;
[CAPTURE.md](../../CAPTURE.md) summarizes findings and consumer recommendations. In corpus.json,
source.licensePath is a logical artifact path with an artifacts/ prefix: strip that prefix and
resolve the resulting path through the snapshot inventory. Preserve the 0001 baseline.
Protocols 5–11 remain the maintained scope. New source/code availability does not authorize future
builtins, Plutus V4 or Dijkstra. Every future exclusion needs a specific captured availability rule.
