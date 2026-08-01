# Cardano Chain

Native TypeScript ledger models, address codecs, transaction construction, and era-aware block
decoding for Cardano.

`@xray-network/xray-cardano-lib-chain` is universal ESM for Node.js and modern browsers. Its runtime uses
`Uint8Array`, `bigint`, and Web Platform APIs directly.

## Installation

```sh
npm install @xray-network/xray-cardano-lib-chain
```

Node.js 20.19 or newer is required.

## Basic usage

```ts
import { Address, NetworkInfo } from "@xray-network/xray-cardano-lib-chain";

const address = Address.from_bech32(
  "addr1u8pcjgmx7962w6hey5hhsd502araxp26kdtgagakhaqtq8sxy9w7g",
);

console.log(address.network_id() === NetworkInfo.mainnet().network_id());
console.log(address.to_hex());
```

The root entry point exposes current ledger models, addresses, native scripts, Plutus data,
transaction builders, hashing, fee calculation, witnesses, and multi-era compatibility types.

## Era entry points

Import from an era entry point when an application only needs that era's runtime and JSON types.

| Entry point | Contents |
| --- | --- |
| `@xray-network/xray-cardano-lib-chain/byron` | Byron encoding, addresses, transactions, witnesses, blocks, and genesis parsing |
| `@xray-network/xray-cardano-lib-chain/shelley` | Shelley blocks, transactions, certificates, and genesis parsing |
| `@xray-network/xray-cardano-lib-chain/allegra` | Allegra blocks, validity intervals, and auxiliary data |
| `@xray-network/xray-cardano-lib-chain/mary` | Mary blocks, transactions, and multi-asset outputs |
| `@xray-network/xray-cardano-lib-chain/alonzo` | Alonzo blocks, scripts, redeemers, and protocol parameters |
| `@xray-network/xray-cardano-lib-chain/babbage` | Babbage blocks, reference inputs, inline data, and reference scripts |
| `@xray-network/xray-cardano-lib-chain/conway` | Conway ledger and governance models |
| `@xray-network/xray-cardano-lib-chain/multi-era` | Era detection and a common view across all supported eras |

For example, an indexer can decode the explicit network envelope without selecting an era first:

```ts
import { MultiEraBlock } from "@xray-network/xray-cardano-lib-chain/multi-era";

export function inspectBlock(bytes: Uint8Array) {
  const block = MultiEraBlock.from_explicit_network_cbor_bytes(bytes);

  return {
    era: block.kind(),
    slot: block.header().slot(),
    transactionCount: block.transaction_bodies().len(),
  };
}
```

## Source layout

```text
src/
├── index.ts
├── address/
├── builder/
├── ledger/
└── era/
    ├── byron/
    ├── shelley/
    ├── allegra/
    ├── mary/
    ├── alonzo/
    ├── babbage/
    ├── conway/
    ├── multi-era/
    └── shared/
```

Each historical era owns its runtime, official-CDDL validation, and JSON types. Multi-era code
depends on the individual era modules, while shared code is limited to common codec, validation,
and model infrastructure. Conway models are reviewed in `src/era/conway/model.ts`; cross-era
models, schemas, and JSON contracts live under `src/era/shared/`. The runtime validates mandatory
fields, discriminators, nested choices, bounds, tagged sets, governance structures, and block
collection invariants while preserving accepted noncanonical CBOR bytes. Captured CDDL is
comparison and conformance evidence rather than an automatic emitter. Chain tests live in
[`test/`](./test/).

## Development

Run workspace commands from the repository root:

```sh
npm --prefix libs/typescript ci
npm --prefix libs/typescript run build
npm --prefix libs/typescript run check
```

`npm --prefix libs/typescript run check` validates captured input provenance, package boundaries, focused codec and
historical-vector tests, security checks, and packed ESM consumers.

See [ADR 0002](../../../../docs/adr/typescript/0002-cryptography-dependency-policy.md) for the security policy and
vulnerability reporting. This package is distributed under the [MIT License](./LICENSE).
