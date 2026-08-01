# TypeScript implementation 0008 instruction

Implementation-Version: v1
Implementation-ID: typescript/0008
Created: 20260731T075521Z
Evidence-Mode: HYBRID
Depends-On: ./0001-IMPL-RESULT.md, ./0004-IMPL-RESULT.md
Provider-Evidence: ../../providers/cardano-cips/0001-cardano-cips/SNAPSHOT.md

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| [`typescript/0001`](./0001-IMPL-RESULT.md) | `IMPLEMENTATION_RESULT` | Yes | Accepted key/address compatibility and package identities |
| [`typescript/0004`](./0004-IMPL-RESULT.md) | `IMPLEMENTATION_RESULT` | Yes | Accepted Conway `Credential` and `GovActionId` owners |
| [`0001-cardano-cips`](https://github.com/xray-network/xray-cardano-lib/blob/main/updates/providers/cardano-cips/0001-cardano-cips/SNAPSHOT.md) | `PROVIDER` | Yes | Immutable CIP-0005, 0016, 0019, 0105, 0129, and 1852 evidence |
| Captured CIP-0019 ABNF and Byron CDDL | `PROVIDER` | Yes | Strict address text/binary boundaries |
| Captured CIP-0105 vector index and four vector files | `PROVIDER` | Yes | Exact governance derivation, key, hash, script, and identifier vectors |
| `libs/typescript/packages/{core,crypto,chain,cip,runtime}/` | `LOCAL` | Yes | Existing Bech32, key, address, credential, and facade owners |

## Objective

Make address and key text identities role-aware and canonical, add typed CIP-1852/CIP-0105
derivation over existing key owners, and expose CIP-0129 governance identifiers only through a
visibly provisional focused surface.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `C001` | Enforce canonical CIP-0005/CIP-0019 address HRPs and encoding families | Strictness fix; raw bytes and explicit unchecked encoding remain | `libs/typescript/packages/chain/src/address/` | CIP-0019 vectors, HRP/network/type matrix, Byron/Base58 |
| `C002` | Add role-aware CIP-0005/CIP-0016 key text codecs and typed CIP-1852/CIP-0105 derivation | Additive APIs returning existing key bindings | `libs/typescript/packages/crypto/src/keys/` | All four CIP-0105 vector documents and path boundaries |
| `C003` | Add provisional CIP-0129 governance credential/action identifiers | Additive focused proposal API; legacy CIP-0105 forms decode-only | `libs/typescript/packages/cip/src/cip129/` | CIP-0129 and CIP-0105 vectors, headers, HRPs, lengths, indexes |

## Required semantics

### C001: canonical addresses

- `Address.from_bech32` must decode and validate both payload and HRP. Shelley payment addresses
  use `addr` only for network tag 1 and `addr_test` for every other accepted network tag. Reward
  addresses use `stake` only for tag 1 and `stake_test` otherwise.
- Reject payment/reward HRP swaps, mainnet/test HRP mismatches, arbitrary HRPs, reserved address
  header kinds, malformed payload lengths, and Byron bytes presented as Bech32.
- `Address.to_bech32()` must select the canonical HRP above. Keep raw-byte construction and expose
  `to_bech32_unchecked(hrp)` for explicit low-level compatibility; retain the old custom-prefix
  overload only as a documented compatibility forwarder to that unchecked method.
- Byron addresses remain canonically Base58. Preserve their accepted raw/CBOR behavior and
  historical vectors.

### C002: role-aware keys and derivation

- Add `CardanoKeyRole` values `Root`, `Account`, `Payment`, `Stake`, `DRep`,
  `ConstitutionalCommitteeCold`, and `ConstitutionalCommitteeHot`.
- Add typed encode/decode functions which return existing `PrivateKey`, `PublicKey`,
  `Bip32PrivateKey`, and `Bip32PublicKey` objects. Map roles to `root`, `acct`, `addr`, `stake`,
  `drep`, `cc_cold`, and `cc_hot`, with `_sk`, `_vk`, `_xsk`, and `_xvk` suffixes.
- Require 32-byte verification keys, 64-byte extended verification keys, and 96-byte extended
  signing keys. Regular `_sk` is 32 bytes; for DRep/CC derived raw signing keys, also support the
  captured CIP-0105 64-byte `_sk` convention. Do not accept 128-byte xprv text encodings.
- Add immutable `Cip1852Path(account, role, index)` for
  `m/1852'/1815'/account'/role/index`. `account` and `index` are `0..0x7fffffff`; role is exactly
  external 0, internal 1, stake 2, DRep 3, CC cold 4, or CC hot 5. The first three components are
  hardened and the final two are not.
- Add private-root derivation, account-private derivation, account-public extraction, and
  account-xpub child derivation helpers. Public derivation starts after the hardened account node
  and must reject hardened children.
- Use the existing Icarus root construction and BIP32-Ed25519 key classes. Do not add mnemonic
  parsing or a competing key nominal type.

### C003: provisional CIP-0129 identifiers

- Own `ProvisionalGovernanceCredentialId` and `ProvisionalGovernanceActionId` only under
  `@xray-network/xray-cardano-lib-cip/cip129`. Add the explicit `./cip129` export, but do not add these
  proposal types to the stable CIP root or aggregate runtime.
- A credential ID is exactly 29 bytes: one header plus the existing 28-byte key/script hash.
  Header high nibbles are CC hot `0`, CC cold `1`, and DRep `2`; low nibbles are key hash `2` and
  script hash `3`. HRPs are respectively `cc_hot`, `cc_cold`, and `drep`; HRP, header role, header
  credential kind, and payload length must all agree.
- Construct from and decode to the accepted Conway `Credential` binding; defensively copy bytes.
- A provisional governance-action ID uses HRP `gov_action` and exactly 33 bytes: a 32-byte
  `TransactionHash` followed by one unsigned index byte. Because the captured proposal defines
  only one-byte examples, canonical encode/decode is deliberately limited to indexes `0..255`;
  conversion from a Conway `GovActionId` above 255 must fail rather than invent a two-byte format.
- Provide a separate decode-only legacy CIP-0105 function for the captured bare 28-byte
  `drep[_script]`, `cc_cold[_script]`, and `cc_hot[_script]` forms. It returns role plus the
  existing `Credential`; no API may canonically re-emit a legacy identifier.

## Implementation steps

1. Centralize the address-kind/network-to-HRP matrix and apply it to strict parse and canonical
   output while preserving explicit unchecked/raw paths.
2. Add role/shape-aware key codecs around existing key classes, then add the typed CIP-1852 path
   and private/account-public derivation helpers.
3. Implement proposal-scoped CIP-0129 owners and the separate legacy decoder.
4. Wire only the approved focused exports and document proposal status prominently.
5. Add captured-vector, malformed, identity, disposal, browser, and packaging tests.

## Validation

- Run every captured CIP-0019 mainnet/testnet address vector; reject every HRP/type/network
  mismatch and prove Byron stays Base58.
- Run every key, xkey, hash, script, legacy identifier, and CIP-0129 identifier assertion in all
  four captured CIP-0105 vector files.
- Cover account/index 0 and `0x7fffffff`, overflow, hardened misuse, roles 0 through 5, private and
  account-xpub derivation equivalence, wrong role HRPs, and all key byte lengths.
- Cover all six CIP-0129 credential headers, reserved nibbles, wrong HRPs, 28/29-byte ambiguity,
  action indexes 0, 17, 255, and rejection of 256 and malformed lengths.
- Assert no provisional binding is exported by the stable CIP root or aggregate runtime.
- Run targeted chain, crypto, CIP, runtime import/browser/pack tests, then
  `npm --prefix libs/typescript run check`.

## Compatibility and human review

Strict address parsing intentionally rejects noncanonical text that raw decoding previously
accepted; raw-byte and explicitly unchecked paths remain. Review key disposal/copy behavior,
private-material exposure, all derivation indexes, and the CIP-0105 64-byte `_sk` compatibility
case. CIP-0129 is Proposed: review its focused naming and one-byte action-index limit before any
later promotion.

## Completion criteria

- Canonical addresses have exactly one conventional text form and Byron remains Base58.
- Key HRPs, payload shapes, paths, and derived values match every captured vector.
- Derivation APIs return existing key identities and enforce every hardened/unhardened boundary.
- Provisional governance identifiers match captured headers/HRPs without entering stable
  aggregate exports; legacy forms are decode-only.
- Targeted tests and `npm --prefix libs/typescript run check` pass.
- The paired result records strictness changes, proposal limitations, and consumed artifacts.

## Out of scope

- BIP39 mnemonic words, seed backup, secure storage, account discovery, or wallet scanning
- CIP-1854/CIP-1855 paths, hardware transports, or text envelopes
- Network/provider lookup or governance registration
- Stable promotion of CIP-0129 or action indexes wider than captured evidence

## Blockers

None.
