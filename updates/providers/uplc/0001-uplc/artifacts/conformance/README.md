# Official Plutus conformance corpus

This directory contains a deterministic JSON capture of every regular file below
`plutus-conformance/test-cases` in
`https://github.com/IntersectMBO/plutus.git` at commit
`91e8c2af9c7bec705b035c6cf8e679c35c4c2ad5`, release tag `1.66.0.0`.

`corpus.json` has schema version 1. Its `entries` are sorted by the UTF-8 bytes of `path`.
Each entry records the path relative to `plutus-conformance/test-cases`, byte length,
lowercase SHA-256 digest, and RFC 4648 padded base64 content. There are exactly 3,013
entries representing 807,307 source bytes; no entry exceeds 19,938 bytes.

Implementation tests must verify the artifact checksum first, then decode every entry and
verify its recorded size and SHA-256 before using it. The `.uplc` inputs and their
`.uplc.expected` and `.uplc.budget.expected` companions are official textual conformance
vectors. A package-private test reader may parse them, but neither the captured Haskell
runner nor any other upstream code may be executed.

The corpus is licensed under Apache-2.0. Its byte-exact license and notice are captured at
`../plutus/plutus-conformance/LICENSE` and
`../plutus/plutus-conformance/NOTICE`.
