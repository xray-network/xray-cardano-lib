# XRAY Cardano Lib

The all-in-one namespace entry point for XRAY Cardano Lib. It exposes each protocol domain through
its owning package without copying, wrapping, or flattening nominal bindings.

The package is universal ESM for Node.js 20.19 or newer and modern browsers.

## Installation

```sh
npm install @xray-network/xray-cardano-lib
```

## Usage

```ts
import { chain, cips, crypto, plutus } from "@xray-network/xray-cardano-lib";

const key = crypto.PrivateKey.generate_ed25519();
const address = chain.Address.from_bech32(
  "stake1u8pcjgmx7962w6hey5hhsd502araxp26kdtgagakhaqtq8squng76",
);
const label = cips.cip67.encode_asset_name_label(222);
const datumSchema = plutus.Data.Integer();

key.dispose();
void [address, label, datumSchema];
```

For concise named imports, consume the owning domain package directly:

```ts
import { Address } from "@xray-network/xray-cardano-lib-chain";
import { Data } from "@xray-network/xray-cardano-lib-plutus";
```

## Public namespaces

| Namespace | Owning package | Contents |
| --- | --- | --- |
| `core` | `@xray-network/xray-cardano-lib-core` | Bytes, CBOR, collections, integers, errors, and network primitives |
| `crypto` | `@xray-network/xray-cardano-lib-crypto` | Keys, signatures, hashes, derivation, encryption, and cryptographic primitives |
| `chain` | `@xray-network/xray-cardano-lib-chain` | Ledger models, addresses, eras, transaction builders, codecs, and operations |
| `cips` | `@xray-network/xray-cardano-lib-cip` | Stable proposal namespaces; provisional CIP-129 remains focused-only |
| `plutus` | `@xray-network/xray-cardano-lib-plutus` | Typed Data, UPLC, blueprints, parameter application, and phase-two valuation |

The aggregate root owns no independent implementation and deliberately has no flat protocol
aliases. This prevents collisions between ledger `chain.PlutusData`, typed `plutus.Data`, and
generic proposal names.

## Development

Run workspace commands from the repository root:

```sh
npm --prefix libs/typescript ci
npm --prefix libs/typescript run check
```

The completion gate validates package boundaries, binding identity, browser-safe ESM output,
behavioral tests, and packed NodeNext/bundler consumers. This package is distributed under the
[MIT License](./LICENSE).
