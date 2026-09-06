# TypeScript implementation 0008 result

Result-Version: v1
Implementation-ID: typescript/0008
Instruction: ./0008-IMPL-INSTR.md
Evidence-Mode: HYBRID

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `C001` | Implemented | Centralized canonical Shelley address HRPs by kind/network, made Bech32 parsing validate HRP and exact payload shape, kept raw construction, and added `to_bech32_unchecked(hrp)` with the old custom-prefix overload forwarding to it. | All 20 captured CIP-0019 mainnet/testnet vectors, mismatch/reserved/length/Byron rejection, raw compatibility, and historical Base58 vectors pass. |
| `C002` | Implemented | Added `CardanoKeyRole`, role-aware codecs returning existing key owners, immutable `Cip1852Path`, Icarus-root/account/private/account-xpub derivation helpers, and aggregate identity-preserving exports. | Every key/xkey entry in all four CIP-0105 vector documents, all roles 0..5, path bounds, public/private equivalence, 64-byte governance `_sk`, 128-byte xprv rejection, and disposal checks pass. |
| `C003` | Implemented | Added focused-only `ProvisionalGovernanceCredentialId`, `ProvisionalGovernanceActionId`, and legacy CIP-0105 decoder under `./cip129`, without stable root or aggregate exports. | Every governance hash/script/legacy/provisional entry in all four CIP-0105 documents, CIP-0129 examples, all six headers, malformed forms, and action indexes 0, 17, and 255 pass; 256 is rejected. |

## Outcome

Shelley address text is now canonical by construction and strict on parse: payment forms use
`addr`/`addr_test`, reward forms use `stake`/`stake_test`, and only network tag 1 uses the mainnet
HRP. Raw construction remains available, custom Bech32 output is explicitly unchecked, and Byron
remains Base58.

Cardano key text codecs now encode roles and key shapes while returning the accepted key owners.
Typed CIP-1852 derivation fixes `1852'/1815'`, validates account/role/index boundaries, and derives
the captured DRep and committee keys identically from private roots and account xpubs.

CIP-0129 remains visibly provisional. Its 29-byte credentials and one-byte-index governance
actions are available only through the focused `cip129` entry point. Captured bare CIP-0105
credentials are decode-only and cannot be canonically re-emitted through that API.

## Inputs consumed

- The current user instruction
- `typescript/0001` and `typescript/0004` accepted implementation results
- `cardano-cips/0001-cardano-cips` provider snapshot
- Captured CIP-0005, CIP-0016, CIP-0019 README/ABNF/Byron CDDL, CIP-0105 vector index and all four
  vector documents, CIP-0129, and CIP-1852 artifacts
- Existing core Bech32, crypto key, chain address/credential/governance-ID, CIP packaging, and
  runtime facade owners
- The adjacent `xray-js` Cardano call sites and installed mnemonic utility, inspected read-only to
  recover the captured vector entropy without adding mnemonic parsing

## Project changes

- Made `Address.from_bech32` validate canonical HRP, address family, network, reserved kinds, and
  exact binary shape.
- Made `Address.to_bech32()` canonical and added `Address.to_bech32_unchecked(hrp)`; retained the
  custom-prefix overload as an unchecked compatibility forwarder.
- Added Cardano role-aware regular/extended private/public key codecs.
- Added `Cip1852Role`, immutable `Cip1852Path`, Icarus entropy root construction, account-private
  derivation, account-public extraction, full private derivation, and account-xpub child derivation.
- Added the focused `./cip129` proposal surface with credential/action owners and a separate
  decode-only legacy CIP-0105 credential function.
- Added complete captured-vector, malformed-input, boundary, disposal, identity, browser, and
  packed-consumer coverage and updated key/address documentation.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `C001` | Canonical Shelley Bech32 HRPs are determined by payment/reward kind and network tag 1 versus all other accepted tags; Byron is never Bech32. | Intentional strictness fix; raw bytes and explicit unchecked output remain. | Replace noncanonical HRPs; use `to_bech32_unchecked` only at an intentional low-level boundary. |
| `C002` | Role-aware codecs cover root/account/payment/stake/DRep/CC keys, and CIP-1852 helpers derive existing BIP32 owners over `m/1852'/1815'/account'/role/index`. | Additive and identity-preserving; mnemonic parsing is not added. | Replace manual HRP/path assembly and retain existing key disposal practices. |
| `C003` | Provisional CIP-0129 credentials encode header plus hash; actions encode transaction hash plus one byte; legacy CIP-0105 identifiers are decode-only. | Additive focused proposal API, intentionally absent from stable roots; action indexes above 255 fail. | Import explicitly from `cip129` and do not treat the proposal surface as stable or invent wider action encodings. |

## Validation

The following checks passed from the TypeScript workspace:

```sh
npm run build
node --test packages/chain/test/chain-logic.test.mjs \
  packages/crypto/test/cardano-keys.test.mjs \
  packages/cip/test/cip129.test.mjs \
  packages/runtime/test/imports.test.mjs \
  packages/runtime/test/browser-package.test.mjs
npm run check
```

- Required focused chain/crypto/CIP/runtime suite: 26 passed.
- Complete built TypeScript suite: 161 passed.
- Packed package smoke suite: 546 intended files, 2,415,564 unpacked bytes; ESM, NodeNext, and
  bundler consumers passed.

## Deviations from instruction

None.

## Remaining human review

Confirm the intentional address strictness break and unchecked escape hatch, all key HRPs and
private-material shape rules, hardened/soft derivation boundaries, CIP-0105 64-byte `_sk`
compatibility, and the focused provisional naming/one-byte CIP-0129 action limit, then decide
whether this result should move from `REVIEW` to `ACCEPTED`.

## Reproducibility

From the repository root with Node.js 20.19 or newer and locked dependencies installed, run
`npm --prefix libs/typescript run check`.
