# Cardano CIP

Application-level implementations of Cardano Improvement Proposals. The package is universal ESM
for Node.js 20.19 or newer and modern browsers.

## Installation

```sh
npm install @xray-network/xray-cardano-lib-cip
```

Import the proposal-specific entry point:

```ts
import { AssetFingerprint } from "@xray-network/xray-cardano-lib-cip/cip14";
import { CIP25Metadata } from "@xray-network/xray-cardano-lib-cip/cip25";
import { CIP36KeyDeregistration } from "@xray-network/xray-cardano-lib-cip/cip36";
import { CIP4 } from "@xray-network/xray-cardano-lib-cip/cip4";
import { CIP8Message, COSESign1Builder } from "@xray-network/xray-cardano-lib-cip/cip8";
```

CIP-129 is still Proposed. Its explicitly provisional governance identifier bindings are
available only from `@xray-network/xray-cardano-lib-cip/cip129`; they are intentionally absent
from the stable CIP root and aggregate runtime.

`AssetFingerprint.from_parts(policyId, assetName)` uses the existing chain `AssetName` and crypto
`ScriptHash` owners and produces the CIP-14 Bech32 display fingerprint. Its 160-bit digest is a
user-facing comparison aid, not a reversible ledger identity or authorization primitive.

`CIP4.calculateChecksum` accepts the canonical CIP-4 input: a lowercase hexadecimal 28-byte
public-key hash. SDKs that expose xpub-based APIs decode or derive the raw public key, hash it, and
pass that hash to this facade.

`CIP8Message.signData` and `CIP8Message.verifyData` provide the CIP-30-compatible message envelope
facade used by application integrations. Lower-level COSE builders remain available for custom
header and detached-payload workflows.

The root package exposes proposal namespaces:

```ts
import { cip4, cip8, cip14, cip25, cip36 } from "@xray-network/xray-cardano-lib-cip";
```

## Entry points

| Entry point | Domain |
| --- | --- |
| `@xray-network/xray-cardano-lib-cip` | CIP-4, CIP-8, CIP-14, CIP-25, and CIP-36 namespaces |
| `@xray-network/xray-cardano-lib-cip/cip14` | User-facing asset fingerprints |
| `@xray-network/xray-cardano-lib-cip/cip4` | Wallet checksum image and text identifier |
| `@xray-network/xray-cardano-lib-cip/cip8` | COSE signing structures, keys, and builders |
| `@xray-network/xray-cardano-lib-cip/cip25` | NFT metadata and label 721 conversion |
| `@xray-network/xray-cardano-lib-cip/cip36` | Voting registration and deregistration metadata |
| `@xray-network/xray-cardano-lib-cip/cip129` | Provisional governance credential/action identifiers |

CIP metadata uses the chain package's existing metadata owners; CIP-8 re-exports the existing core
`Int` and crypto public-key/signature owners by identity. The package uses `Uint8Array` and Web
Platform APIs and is distributed under the [MIT License](./LICENSE).
