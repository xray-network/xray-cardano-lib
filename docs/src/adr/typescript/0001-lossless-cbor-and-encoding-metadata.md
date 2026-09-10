# ADR 0001: Lossless CBOR and encoding metadata

- Status: Accepted
- Date: 2026-07-22

## Context

Real Cardano blocks use valid but non-canonical CBOR. The legacy library preserves integer widths,
container forms, map order, string chunks, tags, and other encoding choices so decoded values can
re-encode byte-for-byte. A high-level CBOR library that immediately produces JavaScript values
cannot satisfy the 85 successful golden block vectors or distinguish preserved from canonical
serialization.

## Decision

- Implement a cursor-based `CborReader` and `CborWriter` over `Uint8Array` in `core` with no
  Cardano domain assumptions.
- Retain token-level details required for reconstruction: integer/tag head width, definite versus
  indefinite lengths, non-minimal length width, byte/text chunks and their widths, optional tags,
  fixed-map order, duplicate entries, and embedded CBOR boundaries.
- Store semantic values in normal fields and encoding metadata in private fields or non-enumerable
  symbols. Metadata is excluded from equality, ordering, fingerprints, and JSON.
- Expose two explicit modes: preserved mode reuses valid decoded metadata; canonical mode ignores
  it and deterministically emits the repository's canonical rules.
- Mutations invalidate only the hints made impossible by that mutation. Unchanged nested values
  retain their own compatible hints.
- Use structural insertion-ordered maps for generated semantic keys and pair arrays for models
  that preserve duplicates. Canonical key order is computed from canonical encoded key bytes where
  the wire rule requires it.
- Enforce complete-input decoding on current `from_cbor_*` APIs and configurable depth/allocation
  limits. Legacy lenient entry points documented in `known-quirks.md` remain separate.
- The generic runtime must support the active tags and forms, including 2, 3, 24, 30, and 258,
  chunked strings, and indefinite arrays/maps.

## Consequences

- Domain models cannot use a third-party decoded object graph as their source of truth.
- Encoding metadata has a memory cost; benchmarks and limits are release gates, not reasons to
  discard fidelity.
- Every codec test needs preserved and canonical expectations, plus mutation cases.
- CIP-25 and deterministic Byron codecs may deliberately use narrower policies registered in the
  overlay registry rather than pretending all packages share one preservation mode.
