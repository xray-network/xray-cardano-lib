# C++ implementation 0004 instruction

Implementation-Version: v1
Implementation-ID: cpp/0004
Created: 20260731T075521Z
Evidence-Mode: HYBRID
Depends-On: ./0001-IMPL-RESULT.md
Provider-Evidence: ../../providers/cardano-cips/0001-cardano-cips/SNAPSHOT.md

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| [`cpp/0001`](./0001-IMPL-RESULT.md) | `IMPLEMENTATION_RESULT` | Yes | Accepted crypto, asset-name, Bech32, CIP component, ownership, and packaging baseline |
| [`0001-cardano-cips`](../../providers/cardano-cips/0001-cardano-cips/SNAPSHOT.md) | `PROVIDER` | Yes | Captured CIP-14 algorithm, HRP, size rules, and eight normative vectors |
| `libs/cpp/include/cardano/crypto/primitives.hpp` and `libs/cpp/include/cardano/chain/builder.hpp` | `LOCAL` | Yes | Existing primitive and `ScriptHash`/`AssetName` owners to reuse |

## Objective

Add a small, strictly typed CIP-14 asset-fingerprint API. The CIP layer owns the semantic
fingerprint and text identity; crypto owns the Blake2b primitive; chain continues to own policy
hashes and asset names.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `C4-HASH1` | Add an owned Blake2b-160 primitive returning exactly 20 bytes. | Additive primitive; existing Blake2b-224/256 behavior is unchanged. | `include/cardano/crypto/primitives.hpp`, `src/crypto/crypto.cpp` | Published algorithm and length vectors |
| `C4-CIP1` | Add `AssetFingerprint`, constructed from a `ScriptHash` plus `AssetName` or parsed from strict CIP-14 text. | New additive nominal type; no competing policy or asset-name owner. | `include/cardano/cip/cip14.hpp`, `src/cip/cip14.cpp` | All captured vectors and round trips |
| `C4-ERR1` | Reject wrong HRP, checksum, case, padding, decoded length, policy length, or asset-name length with stable owned errors. | Generic Bech32 decoding remains permissive about application HRP; the CIP-14 entry point is strict. | CIP-14 owner using core Bech32 | Negative and mutation tests |
| `C4-API1` | Export the same binding through `cardano::cip`, `cip.hpp`, and `cardano.hpp`; update package inventory and docs. | Focused and aggregate entry points preserve one nominal identity. | CIP facade, CMake source list, docs/inventory | Compile identity and installed-consumer tests |

## Implementation steps

1. Add `crypto::blake2b160(ByteSpan) -> Bytes` behind the existing Botan adapter. Assert the
   adapter returns exactly 20 bytes and expose no Botan type in public headers.
2. Define `cardano::cip::AssetFingerprint` as an immutable 20-byte value with:
   `from_asset(const crypto::ScriptHash&, const chain::AssetName&)`,
   `from_bech32(string_view)`, `to_bytes()`, `to_bech32()`, equality, and ordering. Return copies of
   retained bytes.
3. For `from_asset`, hash the exact concatenation
   `policy_id[28] || asset_name[0..32]` with Blake2b-160. Do not hex-decode a display string or add
   length/domain bytes. Encode the digest as lowercase Bech32 with HRP `asset`.
4. For `from_bech32`, require the exact lowercase-normalized HRP `asset`, a valid checksum and
   padding, and exactly 20 decoded bytes. Mixed-case, alternate HRP, trailing data, and overlong or
   short payloads fail.
5. Accept policy and asset inputs only through the existing fixed-size `ScriptHash` and bounded
   `AssetName` owners. Do not add raw `(policy, name)` overloads that bypass their validation.
6. Add `cip14.cpp` to `cardano::cip`, include the focused header from `cip.hpp` and `cardano.hpp`,
   and extend the public API inventory, README feature list, and one focused example.

## Validation

- Reproduce all eight captured CIP-14 vectors, including empty, printable, and arbitrary-byte
  asset names. Assert exact lowercase text and exact 20-byte digest.
- For every vector, parse the text, compare bytes and nominal equality, re-encode it, and construct
  the same value from the typed policy/name pair.
- Add negative tests for a one-bit checksum mutation, `stake` and empty HRPs, mixed case, invalid
  Bech32 alphabet/padding, 19/21-byte payloads, 27/29-byte policies through their owner, and
  33-byte asset names through their owner.
- Add a direct-header compile test and prove the focused, component-umbrella, and aggregate names
  are the same C++ type. Verify release contents contain no provider artifacts.
- Run `cmake --workflow --preset ci`, sanitizers, and hardening from `libs/cpp/`.

## Compatibility and human review

This is a source-compatible additive API. It deliberately tightens only the new CIP-specific text
entry point; the generic core Bech32 codec and caller-selected fixed-hash Bech32 behavior do not
change. Fingerprints are display identifiers, not ledger asset IDs or authentication proofs.

Human review must confirm the concatenation order, Blake2b output size, strict HRP boundary,
defensive ownership, all normative vectors, and absence of duplicate chain nominal types.

## Completion criteria

- Typed construction and strict parsing match every captured CIP-14 vector.
- Invalid domain encodings fail with owned errors and no partial value.
- Focused/component/aggregate identity, API inventory, installed consumer, completion, sanitizer,
  and hardening checks pass.
- A paired result names the exact CIP snapshot artifact consumed.

## Out of scope

- CIP-25/CIP-68 metadata, token-registry lookup, asset discovery, network queries, image/logo
  resolution, or policy-script validation.
- A generic variable-length Blake2b public API, alternative fingerprint HRPs, or changes to ledger
  asset serialization.

## Blockers

None.
