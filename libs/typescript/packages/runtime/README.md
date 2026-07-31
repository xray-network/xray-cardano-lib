# XRAY Cardano Lib

The convenient all-in-one entry point for XRAY Cardano Lib. It exposes the commonly used core values,
cryptographic types, ledger models, addresses, transaction builders, era support, and metadata
types from the domain packages through `@xray-network/xray-cardano-lib`.

The package is universal ESM for Node.js 20.19 or newer and modern browsers.

## Installation

```sh
npm install @xray-network/xray-cardano-lib
```

## Basic usage

```ts
import {
  Address,
  NetworkInfo,
  PrivateKey,
  TransactionHash,
} from "@xray-network/xray-cardano-lib";

const key = PrivateKey.generate_ed25519();
const address = Address.from_bech32(
  "addr1u8pcjgmx7962w6hey5hhsd502araxp26kdtgagakhaqtq8sxy9w7g",
);
const transactionId = TransactionHash.from_raw_bytes(new Uint8Array(32));

console.log(address.network_id() === NetworkInfo.mainnet().network_id());
console.log(transactionId.to_hex());

key.dispose();
```

## Package role

`@xray-network/xray-cardano-lib` is a curated facade over the domain packages:

| Domain | Owning package | Contents |
| --- | --- | --- |
| Core | `@xray-network/cardano-core` | Integers, errors, and network identifiers exposed by the facade |
| Crypto | `@xray-network/cardano-crypto` | Keys, signatures, hashes, and EMIP-3 helpers |
| Chain | `@xray-network/cardano-chain` | Ledger models, addresses, eras, builders, codecs, and operations |
| CIP | `@xray-network/cardano-cip` | CIP-8 signing and CIP-25/CIP-36 metadata |
| Plutus | `@xray-network/cardano-plutus` | Typed Data, UPLC parsing/evaluation, and phase-two valuation |

Applications can depend on a domain package directly when they do not need the complete facade.
Direct imports keep ownership clear and may reduce the surface considered by tooling.

## Source layout

```text
src/
├── index.ts
├── chain/
│   └── index.ts
├── core/
│   └── index.ts
├── crypto/
│   └── index.ts
├── cip/
│   └── index.ts
└── plutus/
    └── index.ts
```

Each domain barrel re-exports its owning package. The root barrel defines the public all-in-one
surface and contains no independent domain implementation.

## Development

Run workspace commands from the repository root:

```sh
npm --prefix libs/typescript ci
npm --prefix libs/typescript run build
npm --prefix libs/typescript test
npm --prefix libs/typescript run check
```

`npm --prefix libs/typescript run check` validates upstream input provenance, package boundaries, behavioral tests,
security checks, and packed ESM consumers. Cross-package contract and downstream tests live in
[`test/`](./test/).

See [ADR 0002](../../../../docs/adr/typescript/0002-cryptography-dependency-policy.md) for the security policy and
vulnerability reporting. This package is distributed under the [MIT License](./LICENSE).
