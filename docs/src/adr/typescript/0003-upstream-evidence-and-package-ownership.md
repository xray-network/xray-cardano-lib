# ADR 0003: Upstream evidence and package ownership

- Status: Accepted
- Date: 2026-07-23

## Context

XRAY Cardano Lib uses immutable upstream snapshots as evidence for reviewed TypeScript implementations.
CDDL and captured source define wire and runtime behavior, but they do not define TypeScript
ownership, public APIs, browser boundaries, or lossless encoding metadata by themselves.

The workspace requires domain-specific package ownership. Protocol-sensitive Plutus behavior and
proposal-specific CIP formats are independent domains rather than generic utilities.

## Decision

- All package TypeScript is ordinary reviewed source colocated with its domain owner. Captured
  upstream material never emits or overwrites runtime source, public exports, JSON contracts, or
  tests.
- Upstream plans map each relevant rule to owned TypeScript, runtime behavior, public API,
  compatibility risk, and focused validation. Ambiguous mappings fail closed.
- Runtime confidence comes from positive, boundary, malformed-input, canonical, preserved-wire,
  package-boundary, and historical-fixture tests.
- Reusable upstream vectors remain in checksummed snapshot artifact trees with pinned provenance
  and license mapping. Published packages never load snapshot artifacts at runtime.
- The published domain packages are `xray-cardano-lib-core`, `xray-cardano-lib-crypto`,
  `xray-cardano-lib-chain`, `xray-cardano-lib-cip`, and `xray-cardano-lib-plutus`, with
  `xray-cardano-lib` as the aggregate runtime.
- `@xray-network/xray-cardano-lib-cip` owns CIP-4, CIP-8, CIP-25, and CIP-36, with focused `./cip4`,
  `./cip8`, `./cip25`, and `./cip36` subpaths. CIP-4 consumes the canonical lowercase hexadecimal
  public-key hash; application SDKs own xpub decoding and key-hash derivation.
- `@xray-network/xray-cardano-lib-plutus` owns typed Data schemas, UPLC, parameter application,
  and raw phase-two valuation. It exposes focused `./data` and `./uplc` subpaths.
- The public UPLC API includes immutable program/term/constant/data types, text parsing, Flat and
  serialized-script codecs, cost-model construction, default machine costs, and budgeted CEK
  evaluation. `applyParamsToScript` and `evaluatePhaseTwoRaw` are exported at the Plutus root.
- Ledger wire models—including `PlutusData`, scripts, redeemers, and `ExUnits`—remain owned by
  `@xray-network/xray-cardano-lib-chain`. Ledger decoding, script resolution, and context
  construction used by raw valuation remain private to the Plutus package.
- `@xray-network/xray-cardano-lib` re-exports curated domain bindings by identity and owns no competing
  nominal implementation.
- Domain packages use universal ESM, `Uint8Array`, `bigint`, Web Platform APIs, and the dependency
  direction declared by their project references.

## Consequences

Ownership follows protocol domains and is visible in package names and subpaths. Applications may
import focused packages directly or use the aggregate runtime. AST values are immutable structural
data; generic UPLC evaluation is explicitly budgeted, and callers are responsible for choosing
protocol-appropriate costs and semantics.

Snapshot preparation and implementation require direct evidence review rather than generated
source. There is no repository CDDL generator, overlay registry, API manifest generator, or generic
utilities package.
