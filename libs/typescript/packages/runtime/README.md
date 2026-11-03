# XRAY Cardano Lib

`@xray-network/xray-cardano-lib` is the all-in-one namespace entry point for XRAY Cardano Lib.
It exposes each protocol domain through
its owning package without copying, wrapping, or flattening nominal bindings.

## Installation

```sh
npm install @xray-network/xray-cardano-lib
```

## Requirements

ESM-only, with TypeScript declarations included. Use Node.js 20.19 or newer, or a modern browser
with `Uint8Array`, `bigint`, and Web Platform APIs. Key generation also requires
`globalThis.crypto.getRandomValues`. No CommonJS, native addon, or WebAssembly build is shipped.

## Quickstart

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

## Entry points

The supported entry point is `@xray-network/xray-cardano-lib`. Import focused APIs from their
owning domain package when you do not need the aggregate namespaces.

## Public namespaces

| Namespace | Owning package                          | Contents                                                                       |
| --------- | --------------------------------------- | ------------------------------------------------------------------------------ |
| `core`    | `@xray-network/xray-cardano-lib-core`   | Bytes, CBOR, collections, integers, errors, and network primitives             |
| `crypto`  | `@xray-network/xray-cardano-lib-crypto` | Keys, signatures, hashes, derivation, encryption, and cryptographic primitives |
| `chain`   | `@xray-network/xray-cardano-lib-chain`  | Ledger models, addresses, eras, transaction builders, codecs, and operations   |
| `cips`    | `@xray-network/xray-cardano-lib-cip`    | Stable proposal namespaces; provisional CIP-129 remains focused-only           |
| `plutus`  | `@xray-network/xray-cardano-lib-plutus` | Typed Data, UPLC, blueprints, parameter application, and phase-two valuation   |

The aggregate root owns no independent implementation and deliberately has no flat protocol
aliases. This prevents collisions between ledger `chain.PlutusData`, typed `plutus.Data`, and
generic proposal names.

Transaction inspection bindings exposed through `chain` are the exact nominal bindings from the
chain package. Standalone multi-era transaction bodies retain an unknown era until authoritative
block context supplies one, and intrinsic accessors preserve original CBOR while exposing exact
ledger values without UTxO resolution or signedness inference.

## Development

Run workspace commands from the repository root:

```sh
npm --prefix libs/typescript ci
npm --prefix libs/typescript run check
```

The completion gate validates package boundaries, binding identity, browser-safe ESM output,
behavioral tests, and packed NodeNext/bundler consumers.

## Documentation

See the [TypeScript package overview](https://github.com/xray-network/xray-cardano-lib/blob/main/libs/typescript/README.md) and
[architecture decisions](https://github.com/xray-network/xray-cardano-lib/blob/main/docs/README.md).

## License

MIT. See [LICENSE](./LICENSE).
