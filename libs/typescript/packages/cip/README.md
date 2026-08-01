# Cardano CIP

Application-level implementations of Cardano Improvement Proposals. The package is universal ESM
for Node.js 20.19 or newer and modern browsers.

## Installation

```sh
npm install @xray-network/xray-cardano-lib-cip
```

Import the proposal-specific entry point:

```ts
import { CIP25Metadata } from "@xray-network/xray-cardano-lib-cip/cip25";
import { CIP36KeyDeregistration } from "@xray-network/xray-cardano-lib-cip/cip36";
import { COSESign1Builder } from "@xray-network/xray-cardano-lib-cip/cip8";
```

The root package exposes proposal namespaces:

```ts
import { cip25, cip36, cip8 } from "@xray-network/xray-cardano-lib-cip";
```

## Entry points

| Entry point | Domain |
| --- | --- |
| `@xray-network/xray-cardano-lib-cip` | CIP-8, CIP-25, and CIP-36 namespaces |
| `@xray-network/xray-cardano-lib-cip/cip8` | COSE signing structures, keys, and builders |
| `@xray-network/xray-cardano-lib-cip/cip25` | NFT metadata and label 721 conversion |
| `@xray-network/xray-cardano-lib-cip/cip36` | Voting registration and deregistration metadata |

CIP metadata uses the chain package's existing metadata owners; CIP-8 re-exports the existing core
`Int` and crypto public-key/signature owners by identity. The package uses `Uint8Array` and Web
Platform APIs and is distributed under the [MIT License](./LICENSE).
