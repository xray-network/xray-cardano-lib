# C++ implementation 0005 instruction

Implementation-Version: v1
Implementation-ID: cpp/0005
Created: 20260731T075521Z
Evidence-Mode: HYBRID
Depends-On: ./0001-IMPL-RESULT.md
Provider-Evidence: ../../providers/cardano-cips/0001-cardano-cips/SNAPSHOT.md

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| [`cpp/0001`](./0001-IMPL-RESULT.md) | `IMPLEMENTATION_RESULT` | Yes | Accepted address, Bech32, key, BIP32, fixed-hash, security, and ownership baseline |
| [`0001-cardano-cips`](https://github.com/xray-network/xray-cardano-lib/blob/main/.xray/updates/providers/cardano-cips/0001-cardano-cips/SNAPSHOT.md) | `PROVIDER` | Yes | Captured CIP-5, CIP-16, CIP-19, CIP-105, CIP-129, and CIP-1852 text and CIP-105 vectors |
| `libs/cpp/include/cardano/crypto/keys.hpp` and `libs/cpp/include/cardano/chain/address.hpp` | `LOCAL` | Yes | Existing nominal key/address owners to extend without replacement |

## Objective

Add role-aware text identities and typed Cardano HD paths, make domain address parsing strict, and
provide a visibly provisional CIP-129 governance identifier API. Existing key, hash, address, and
transaction-hash types remain the sole byte owners.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `C5-KEY1` | Extend the existing move-only `PrivateKey` owner to distinguish 32-byte normal and 64-byte extended Ed25519 secrets, then implement CIP-5/CIP-16 role-aware Bech32 encoding and strict decoding for normal and extended keys. | Existing normal-key behavior and generic `ed25519_*`, `xprv`, and `xpub` methods remain; role methods return the same nominal key types. | `include/cardano/crypto/keys.hpp`, `src/crypto/crypto.cpp`, `include/cardano/crypto/identity.hpp`, `src/crypto/identity.cpp` | Prefix/length/type, signing/public-key, clearing, and negative tests |
| `C5-HD1` | Add checked CIP-1852 paths and derive roles 0–5, including CIP-105 DRep and committee keys. | Existing raw `derive(uint32_t)` remains available; typed paths cannot encode an invalid hardening pattern. | `include/cardano/crypto/derivation.hpp`, `src/crypto/derivation.cpp` | Path, public/private equivalence, boundary, and captured vector tests |
| `C5-ADR1` | Make the domain address Bech32 entry point enforce CIP-19 address class and network HRP. | Valid addresses are unchanged; a clearly named compatibility parser retains the old HRP-agnostic payload path. | `include/cardano/chain/address.hpp`, `src/chain/chain.cpp` | Mainnet/testnet, payment/reward, long-address, and wrong-HRP tests |
| `C5-GOV1` | Add strict, provisional CIP-129 committee/DRep and governance-action identifiers plus explicit decode-only CIP-105 legacy hash identifiers. | Proposal-only API is isolated below `cardano::cip::experimental::cip129`; no canonical output emits the legacy form. | `include/cardano/cip/cip129.hpp`, `src/cip/cip129.cpp` | Header matrix, HRP, vector, round-trip, and rejection tests |
| `C5-API1` | Export stable identity/derivation additions through their owning facades, install CIP-129 only as a focused experimental header, document status/security boundaries, and extend inventories. | Additive except strict rejection of mislabelled address text; provisional bindings do not enter `cip.hpp` or `cardano.hpp`. | Crypto/chain facades, focused CIP header, docs/inventory | Direct-header, nominal-identity, aggregate-absence, and installed-consumer tests |

## Exact identity contract

- `KeyTextRole` contains `root`, `account`, `payment`, `stake`, `drep`, `cc_cold`, and `cc_hot`.
  Role encoding uses the captured role stem followed by `_vk`, `_sk`, `_xvk`, or `_xsk`.
  Normal public/private payloads are 32 bytes, extended public payloads are 64 bytes, and extended
  private payloads are 96 bytes. For the DRep and committee roles only, the captured CIP-105
  vectors also use a 64-byte extended private scalar as `*_sk`; that form returns the same
  `PrivateKey` nominal owner after this sequence extends it with a non-secret
  `PrivateKeyForm::{normal, extended}` discriminator, and is distinct from 96-byte `*_xsk`. The
  extended representation must preserve
  the existing move-only secret boundary: byte export is defensive, `public_key()` and `sign()`
  dispatch to the matching normal or extended Ed25519 operation, and move, `clear()`, and
  destruction wipe the complete active secret buffer. A parser receives the expected key form and
  role; it rejects a valid encoding of another form or role rather than inferring authorization
  from its text.
- `Cip1852Role` is exactly `external=0`, `internal=1`, `staking=2`, `drep=3`,
  `cc_cold=4`, and `cc_hot=5`. A full private path is
  `m / 1852' / 1815' / account' / role / index`: the first three components are hardened and the
  final two are soft. Account and index inputs must be below `2^31`. Public account derivation
  accepts only the role/index suffix.
- Strict Shelley payment address HRPs are `addr` for network tag 1 and `addr_test` otherwise;
  reward address HRPs are `stake` and `stake_test` by the same rule. The payload's header must
  identify the matching payment/reward class. Byron continues to use Base58.
- A provisional CIP-129 credential identifier is exactly one header byte plus one 28-byte
  credential hash. Header high nibbles are committee hot `0`, committee cold `1`, and DRep `2`;
  low nibbles are key hash `2` and script hash `3`. HRPs must be `cc_hot`, `cc_cold`, and `drep`
  respectively and agree with the header.
- A provisional `gov_action` identifier is `TransactionHash[32] || index`. This sequence supports
  only the captured, unambiguous one-octet index range `0..255`; larger indexes fail closed pending
  a later evidence record. Canonical text uses HRP `gov_action`.

## Implementation steps

1. Extend the existing `PrivateKey` binding with explicit `from_normal_bytes` and
   `from_extended_bytes` factories and `form()` inspection. Keep the existing `from_bytes`,
   `from_hex`, `from_bech32`, `generate`, and `to_bech32` behavior as 32-byte normal-key
   compatibility paths; extended import/export occurs only through the explicit factory and
   role-aware codecs. Add
   `Bip32PrivateKey::to_raw_key()` returning the 64-byte extended form without its chain code.
   Both private-key forms remain noncopyable and use one securely cleared owner.
2. Add role-aware free functions that consume or return existing `PublicKey`, `PrivateKey`,
   `Bip32PublicKey`, and `Bip32PrivateKey` bindings. Do not subclass keys, duplicate secret
   storage, or expose a variant that copies private keys.
3. Validate HRP before constructing a key, then validate exact payload length and the owning key's
   structural constraints. Encoding is lowercase and uses only the caller-selected enumerated
   role, never an arbitrary string prefix.
4. Define `Cip1852Path` and `Cip1852Account` value objects with checked factories and explicit
   `derive_private`/`derive_public` operations. Reuse the accepted BIP32 primitive for each index;
   do not reimplement HMAC or curve arithmetic.
5. Reproduce CIP-105 vector results from captured already-derived hex and Bech32 keys, hashes, and
   identifiers. Captured mnemonic words are evidence context only and must not enter tests, logs,
   source, or a new mnemonic API.
6. Change `Address::from_bech32` to the strict CIP-19 contract. Name the old behavior
   `from_bech32_payload_compatible`, mark it compatibility-only in documentation, and keep
   `Address::from_bytes` as the preferred raw-wire entry point. Generic `core::decode_bech32` and
   caller-HRP fixed hashes remain unchanged.
7. Build provisional governance identifiers in the CIP component from the existing
   `Ed25519KeyHash`/`ScriptHash` and
   `TransactionHash` owners. Parsing validates checksum, case, HRP, length, header reserved bits,
   and role/kind agreement. Expose legacy CIP-105 credential-hash parsing only through
   `parse_legacy_cip105`; it returns typed bytes but has no `to_legacy_bech32`.
8. Put every CIP-129 declaration under `cardano::cip::experimental::cip129` and label proposal
   status in API docs. Install the focused header but do not include it from `cardano/cip/cip.hpp`
   or `cardano/cardano.hpp`. Do not let provisional identifiers become implicit address,
   certificate, or builder conversions.

## Validation

- Cover every key role and each normal/extended public/private form, including the captured
  DRep/committee 64-byte `*_sk` cases, exact payload sizes, wrong-role/form HRPs, checksums, mixed
  case, padding, and secret move/clear behavior.
- Test account/index values `0`, `2^31-1`, `2^31`, and `UINT32_MAX`; all roles 0–5; the exact
  hardened/soft sequence; and public/private suffix agreement. Reproduce the applicable captured
  CIP-105 key/hash/identifier vectors without mnemonic parsing.
- Cover all Shelley address header classes and network tags, matching and mismatched payment/reward
  HRPs, reserved network tags using `_test`, Byron rejection at the Bech32 entry point, and the
  explicit compatibility parser.
- Cover all six CIP-129 role/kind headers, reserved nibbles, cross-role HRPs, 28-byte boundaries,
  action indexes 0, 17, 255, and rejection of 256. Prove legacy forms are decode-only.
- Compile the focused CIP-129 header directly and prove its types are absent from the stable CIP
  umbrella and aggregate header.
- Run completion, sanitizer, hardening, inventory, component, aggregate, installed-content, and
  provider-integrity gates.

## Compatibility and human review

Raw bytes and generic Bech32 utilities remain available, but domain entry points no longer accept a
misleading HRP. CIP-129 is proposed evidence and is intentionally provisional, focused, and
fail-closed; a later accepted specification may require a new API/version.

Human review must verify secret ownership, prefix/form mapping, hardening arithmetic, address HRP
classification, governance header bits, legacy isolation, and vector handling.

## Completion criteria

- Role key encodings, typed derivation, strict addresses, and provisional identifiers satisfy the
  exact contracts above.
- Both normal and extended `PrivateKey` forms sign and derive their public key correctly, clear all
  active secret bytes, remain noncopyable, and reuse one nominal owner.
- No existing nominal key/hash/address owner is duplicated.
- Every captured applicable vector and all required negative cases pass.
- The paired result records proposal status, compatibility behavior, and exact inputs consumed.

## Out of scope

- Mnemonic parsing/generation, wallet discovery, account gap scanning, key storage, hardware
  signing, CIP-3/CIP-1854/CIP-1855, CIP-30, or CIP-95.
- Canonical support for CIP-129 governance-action indexes above 255, implicit migration of legacy
  identifiers, network lookups, or changes to certificate wire models.

## Blockers

None.
