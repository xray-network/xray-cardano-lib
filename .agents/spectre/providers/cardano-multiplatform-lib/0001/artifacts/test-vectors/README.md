# Cardano test vectors

This directory contains externally sourced block and genesis JSON vectors used
by the test suite.

[`manifest.json`](./manifest.json) is the authoritative inventory.
For every copied asset it records:

- `path`: the stable logical path inside this artifact corpus;
- `fixturePath`: the tracked path in this repository;
- `sha256`: the checksum of the stored bytes; and
- size, storage format, expected result, and (for golden blocks) the fixture's
  original external source revision.

Keep fixture bytes immutable; refresh the inventory and provenance together
when intentionally updating the corpus.

The byte-exact upstream [`PROVENANCE.md`](./PROVENANCE.md) records the Dolos and Pallas source
locations, revisions, refresh policy, era coverage, and known malformed Conway fixture. Its
`mainnet_blocks/` source directory is stored as `blocks/mainnet/` in this corpus; `pallas/` is
stored as `blocks/pallas/`.
[`PROVENANCE.json`](./PROVENANCE.json)
checksums that document and maps both fixture sets to their pinned source revisions. Dolos and
Pallas distribute these fixtures under Apache-2.0; the required license text is included as
[`LICENSE-APACHE-2.0.txt`](./LICENSE-APACHE-2.0.txt). The enclosing CML snapshot retains its
attribution in [`artifacts/legal/`](../../legal/).
