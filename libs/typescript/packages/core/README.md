# Cardano Core

Browser-safe foundational types and codecs shared by the Cardano Lib packages.
`@xray-network/cardano-core` provides byte utilities, lossless CBOR handling, ordered
collections, numeric primitives, network identifiers, and consistent errors without depending on
another workspace package.

The package is universal ESM for Node.js 20.19 or newer and modern browsers.

## Installation

```sh
npm install @xray-network/cardano-core
```

## Basic usage

The root entry point exposes the complete foundational API:

```ts
import {
  Int,
  bytesToHex,
  decodeCbor,
  encodeCbor,
} from "@xray-network/cardano-core";

const integer = Int.new(42n);
const encoded = integer.to_cbor_bytes();
const decoded = decodeCbor(encoded);

console.log(bytesToHex(encodeCbor(decoded)));
```

Bech32 is intentionally available through its focused entry point:

```ts
import {
  decodeBech32,
  encodeBech32,
} from "@xray-network/cardano-core/bech32";

const encoded = encodeBech32("example", Uint8Array.of(1, 2, 3));
const { prefix, bytes } = decodeBech32(encoded);
```

## API domains

| Domain | Main exports | Responsibility |
| --- | --- | --- |
| Bytes | `ByteArray`, `SecureRandomSource`, `bytesToHex`, `hexToBytes`, `bytesEqual` | Portable byte types, copying, comparison, conversion, and length checks |
| CBOR | `CborValue`, `decodeCbor`, `encodeCbor`, `decodeEmbeddedCbor` | Lossless decoding, preserved or canonical encoding, and decoder limits |
| Collections | `NonEmptyVec`, `NonEmptyMap`, `OrderedMap`, `PairMap` | Explicit non-empty, ordered, structural-key, and duplicate-key semantics |
| Encoding | `encodeBech32`, `decodeBech32` | Bech32 encoding through the `/bech32` entry point |
| Errors | `CardanoError`, `CardanoBoundsError`, `DeserializeError`, `CardanoResult` | Stable error codes, paths, offsets, and result helpers |
| Numbers | `Int`, `BigInteger`, integer bounds, `asInt64`, `asUint64` | Cardano integer ranges and CBOR-aware numeric values |
| Network | `ProtocolMagic` and network magic constants | Validated protocol magic values and CBOR conversion |
| Shared conventions | `Cloneable`, `Equatable`, `Comparator`, `cloneValue` | Common contracts used across domain packages |

`decodeCbor` rejects trailing data and supports configurable depth, collection, string, and token
limits. Decoded values retain wire-level details such as integer head width, definite versus
indefinite collections, string chunks, and source spans. `encodeCbor` preserves those details by
default; pass `{ mode: "canonical" }` when canonical CBOR is required.

`OrderedMap` replaces an existing structurally equal key while retaining insertion order.
`PairMap` retains duplicate keys, which is necessary for faithfully representing some decoded
Cardano data.

## Entry points

| Entry point | Contents |
| --- | --- |
| `@xray-network/cardano-core` | Bytes, CBOR, collections, errors, numbers, network values, and shared conventions |
| `@xray-network/cardano-core/bech32` | Bech32 encoder and decoder |

## Source layout

```text
src/
├── index.ts
├── bytes/
│   ├── index.ts
│   ├── types.ts
│   └── utilities.ts
├── cbor/
│   ├── codec.ts
│   ├── index.ts
│   ├── limits.ts
│   └── types.ts
├── collections/
│   ├── index.ts
│   ├── key-equality.ts
│   ├── non-empty.ts
│   ├── ordered-map.ts
│   └── pair-map.ts
├── encoding/
│   ├── bech32.ts
│   └── index.ts
├── errors/
│   ├── cardano-error.ts
│   ├── deserialize-error.ts
│   ├── index.ts
│   ├── result.ts
│   └── types.ts
├── network/
│   ├── index.ts
│   └── protocol-magic.ts
├── numbers/
│   ├── big-integer.ts
│   ├── bounds.ts
│   ├── index.ts
│   └── int.ts
└── shared/
    ├── conventions.ts
    ├── index.ts
    └── types.ts
```

Domain barrels define ownership inside the package. The root barrel deliberately lists the public
surface, while implementation files remain private package internals.

## Development

Run workspace commands from the repository root:

```sh
npm --prefix libs/typescript ci
npm --prefix libs/typescript run build
npm --prefix libs/typescript test
npm --prefix libs/typescript run check
```

`npm --prefix libs/typescript run check` validates TypeScript, package boundaries, browser-safe ESM output, tests, security
rules, and packed-package imports.

## Security

Decoding untrusted CBOR should use limits appropriate to the surrounding application. Keep the
default limits or lower them when inputs are expected to be small. Treat malformed encodings and
range failures as untrusted input errors rather than retryable transport failures.

Please report suspected vulnerabilities according to
[ADR 0002](../../../../docs/adr/typescript/0002-cryptography-dependency-policy.md). Do not include private keys,
seed phrases, or production signing material in a report.

## License

Cardano Core is available under the [MIT License](./LICENSE).
