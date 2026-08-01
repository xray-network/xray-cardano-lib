# TypeScript implementation

Status: Maintained

This directory owns the shared TypeScript workspace configuration and package lock. Its six
published packages, including their source and tests, live under `packages/`.

## Packages

| Package | Responsibility |
| --- | --- |
| [`@xray-network/xray-cardano-lib-core`](./packages/core/README.md) | Core CBOR, encoding, collections, numbers, errors, and network primitives |
| [`@xray-network/xray-cardano-lib-crypto`](./packages/crypto/README.md) | Hashes, keys, signatures, encryption, and cryptographic primitives |
| [`@xray-network/xray-cardano-lib-chain`](./packages/chain/README.md) | Cardano eras, ledger models, addresses, validation, and transaction building |
| [`@xray-network/xray-cardano-lib-cip`](./packages/cip/README.md) | Supported Cardano Improvement Proposal APIs |
| [`@xray-network/xray-cardano-lib-plutus`](./packages/plutus/README.md) | Plutus Data, UPLC, cost models, contexts, and evaluation |
| [`@xray-network/xray-cardano-lib`](./packages/runtime/README.md) | Unified public runtime facade |

## Development

```sh
npm ci
npm run build
npm test
npm run check
```

Run these commands from `libs/typescript/`, or use `npm --prefix libs/typescript <command>` from the
repository root.

The workspace owns its npm manifest, lockfile, project references, test discovery, and package
smoke tests. The repository root does not proxy these commands.

## Implementation history

Numbered implementation updates and provider evidence are outside this source workspace:

- Instructions and results: [`updates/implementations/typescript/`](../../updates/implementations/typescript/)
- Implementation status: [`STATUS.md`](../../updates/implementations/typescript/STATUS.md)
- Shared provider evidence: [`updates/providers/`](../../updates/providers/)
- Implementation workflow: [`TEMPLATE_IMPL.md`](../../updates/TEMPLATE_IMPL.md)

Provider evidence and accepted results define declared inputs. TypeScript packages own their
language representation, public API, tests, and validation evidence.
