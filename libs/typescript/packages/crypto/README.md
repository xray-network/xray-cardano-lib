# `@xray-network/cardano-crypto`

Native ESM cryptography and Cardano key types for Cardano Lib. The package runs locally in Node.js
and modern browsers.

It provides:

- normal and extended Ed25519 keys and signatures;
- Cardano Ed25519-BIP32 private/public keys and derivation;
- fixed-size Cardano hash and verification-key wrappers;
- Blake2b-224, Blake2b-256, and SHA3-256 helpers;
- cryptographically secure random bytes through Web Crypto;
- EMIP-3 password encryption and decryption; and
- the legacy Daedalus key operations required by Byron compatibility code.

## Install

```sh
npm install @xray-network/cardano-crypto
```

The package is ESM-only and requires Node.js 20.19 or newer, or a browser/runtime with the Web
Crypto `getRandomValues` API.

## Sign and verify

```ts
import { PrivateKey } from "@xray-network/cardano-crypto";

const message = new TextEncoder().encode("Cardano Lib");
const privateKey = PrivateKey.generate_ed25519();
const publicKey = privateKey.to_public();
const signature = privateKey.sign(message);

try {
  if (!publicKey.verify(message, signature)) {
    throw new Error("signature verification failed");
  }
} finally {
  signature.dispose();
  publicKey.dispose();
  privateKey.dispose();
}
```

`PrivateKey.generate_ed25519()` fails closed when secure system randomness is unavailable.
Workspace tests inject deterministic randomness at an internal boundary; application code should
use the public secure system source.

## Derive a Cardano BIP32 key

`from_bip39_entropy` accepts already-decoded BIP-39 entropy; this package does not parse mnemonic
words.

```ts
import { Bip32PrivateKey } from "@xray-network/cardano-crypto";

const entropy = Uint8Array.from([
  0xdf, 0x9e, 0xd2, 0x5e, 0xd1, 0x46, 0xbf, 0x43,
  0x33, 0x6a, 0x5d, 0x7c, 0xf7, 0x39, 0x59, 0x94,
]);
const root = Bip32PrivateKey.from_bip39_entropy(entropy, new Uint8Array());
const purpose = root.derive(0x8000_0000 + 1852);

try {
  const publicKey = purpose.to_public();
  try {
    console.log(publicKey.to_bech32());
  } finally {
    publicKey.dispose();
  }
} finally {
  purpose.dispose();
  root.dispose();
}
```

Hardened derivation requires a private key. `Bip32PublicKey.derive` rejects hardened indexes.

## Hash Cardano data

```ts
import {
  TransactionHash,
  blake2b256,
} from "@xray-network/cardano-crypto";

const transactionBody = Uint8Array.of(0xa0);
const hash = TransactionHash.from_raw_bytes(blake2b256(transactionBody));

console.log(hash.to_hex());
```

Fixed-size wrappers validate their byte length and defensively copy input and output byte arrays.

## EMIP-3 encryption

The EMIP-3 functions use hexadecimal strings because that is the historical Cardano wallet
interface.

```ts
import {
  emip3_decrypt_with_password,
  emip3_encrypt_with_password,
  secureRandomBytes,
} from "@xray-network/cardano-crypto";

const toHex = (bytes: Uint8Array): string =>
  [...bytes].map((byte) => byte.toString(16).padStart(2, "0")).join("");

const password = toHex(new TextEncoder().encode("correct horse battery staple"));
const salt = toHex(secureRandomBytes(32));
const nonce = toHex(secureRandomBytes(12));
const plaintext = toHex(new TextEncoder().encode("wallet secret"));

const encrypted = emip3_encrypt_with_password(password, salt, nonce, plaintext);
const decrypted = emip3_decrypt_with_password(password, encrypted);

if (decrypted !== plaintext) throw new Error("EMIP-3 round trip failed");
```

Use a fresh random salt and nonce for every encryption. EMIP-3's fixed KDF parameters are retained
for format compatibility; they are not a recommendation for a new password-storage design.

## Public API

The supported package entry point is `@xray-network/cardano-crypto`.

| Area | Public values |
|---|---|
| Keys | `PrivateKey`, `PublicKey`, `Ed25519Signature`, `Bip32PrivateKey`, `Bip32PublicKey` |
| Hashes | `Ed25519KeyHash`, `ScriptHash`, `TransactionHash`, `GenesisDelegateHash`, `GenesisHash`, `AuxiliaryDataHash`, `PoolMetadataHash`, `VRFKeyHash`, `BlockBodyHash`, `BlockHeaderHash`, `DatumHash`, `ScriptDataHash`, `VRFVkey`, `KESVkey`, `NonceHash`, `AnchorDocHash` |
| Digests | `blake2b224`, `blake2b256`, `sha3_256` |
| Randomness | `secureRandomBytes`, `systemSecureRandomSource` |
| Encryption | `emip3_encrypt_with_password`, `emip3_decrypt_with_password` |
| Byron compatibility | `LegacyDaedalusPrivateKey`, `legacyPublicKey`, `legacySign` |

Internal source folders are not package subpaths. Importing emitted files directly is unsupported.

## Source map

```text
src/
  index.ts
  hashes/
    index.ts
    fixed-bytes.ts
    types.ts
  keys/
    index.ts
    ed25519.ts
  primitives/
    index.ts
    crypto.ts
    random.ts
  encryption/
    index.ts
    emip3.ts
  byron/
    index.ts
    abor.ts
    legacy.ts
    proxy.ts
```

| Domain | Responsibility |
|---|---|
| `hashes` | Fixed-size byte wrappers and Cardano hash owners |
| `keys` | Ed25519, extended Ed25519, and Cardano BIP32 key lifecycles |
| `primitives` | Noble-library boundaries, byte arithmetic, digests, signatures, KDFs, AEAD, and secure randomness |
| `encryption` | EMIP-3 wire layout and password-based encryption |
| `byron` | Internal ABOR, legacy Daedalus, and Byron proxy-certificate compatibility |
| `index.ts` | Stable public package barrel |

`src/hashes/types.ts` is ordinary reviewed source. Hash-size or ownership changes require focused
vectors, API compatibility review, and direct comparison with captured ledger grammar when
applicable.

## Security notes

- JavaScript cannot guarantee secret zeroization. Garbage collection, engine internals,
  dependency calls, and returned copies may retain bytes after a key is disposed.
- `dispose()` overwrites buffers owned by key and signature objects as a best effort. It cannot
  erase copies held elsewhere.
- Random key generation exclusively uses `globalThis.crypto.getRandomValues` unless an explicit
  randomness source is supplied. It never falls back to `Math.random`.
- Authentication failures during EMIP-3 decryption are rejected; callers should not reveal
  password or ciphertext details in error messages.
- Cryptographic dependencies are exactly pinned in the workspace lockfile. Dependency pinning and
  tests are not substitutes for an independent audit.

See the [security and dependency policy](../../../../docs/adr/typescript/0002-cryptography-dependency-policy.md)
and [cryptography decision](../../../../docs/adr/typescript/0004-cryptography-primitives.md) for the
current threat model and review status.

## Development

From the repository root:

```sh
npm --prefix libs/typescript ci
npx tsc -b libs/typescript/packages/crypto/tsconfig.json
npm --prefix libs/typescript run check
```

The complete TypeScript gate checks captured CDDL, TypeScript declarations, browser boundaries,
published package contents, cryptographic vectors, malformed inputs, mutation probes, and
installed-consumer behavior.

## License

Cardano Lib is distributed under the MIT license. See [LICENSE](./LICENSE). Runtime cryptography
dependencies retain their own licenses and notices.
