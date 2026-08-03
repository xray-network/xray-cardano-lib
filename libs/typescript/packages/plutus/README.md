# Cardano Plutus

Typed Plutus Data, UPLC parsing and serialization, budgeted CEK evaluation, script parameter
application, and raw phase-two transaction valuation for Node.js and modern browsers.

## Installation

```sh
npm install @xray-network/xray-cardano-lib-plutus
```

## UPLC

```ts
import {
  encodeProgramEnvelope,
  evaluateProgram,
  parseUplcText,
} from "@xray-network/xray-cardano-lib-plutus/uplc";

const program = parseUplcText("(program 1.0.0 (con unit ()))");
const script = encodeProgramEnvelope(program);
const result = evaluateProgram(
  program,
  [],
  { cpu: 10_000_000n, memory: 10_000_000n },
);
```

The root entry point exports the same UPLC API together with `Data`, `applyParamsToScript`, and
`evaluatePhaseTwoRaw`. Generic CEK evaluation is explicitly budgeted and does not perform ledger
phase-one validation.

`SerializedPlutusScript.from_raw_flat`, `from_single_cbor`, and `from_double_cbor` explicitly own
the three serialized-script forms. Each constructor validates an exact Flat program and its exact
CBOR nesting; `to_raw_flat`, `to_single_cbor`, and `to_double_cbor` convert without envelope
guessing.

`evaluatePhaseTwo(transaction, resolvedInputs, costModels, ...)` accepts the existing chain-owned
ledger bindings and returns immutable evaluations paired with chain-owned `RedeemerWitnessKey`
values. `evaluatePhaseTwoRaw` remains available for byte-oriented compatibility.

## Typed Data

```ts
import { Data } from "@xray-network/xray-cardano-lib-plutus/data";

const payment = Data.Object({
  owner: Data.Bytes({ minLength: 28, maxLength: 28 }),
  amount: Data.Integer({ minimum: 0 }),
});

const voidSchema = Data.Void();
Data.to(undefined, voidSchema); // "d87980"
```

`Data.Void()` is the typed schema for constructor alternative zero with no fields. The existing
lowercase `Data.void()` helper continues to return that raw CBOR directly.

Ledger wire types such as `PlutusData`, scripts, redeemers, and `ExUnits` remain owned by
`@xray-network/xray-cardano-lib-chain`.

## Entry points

| Entry point | Domain |
| --- | --- |
| `@xray-network/xray-cardano-lib-plutus` | Data, UPLC, parameter application, and phase-two valuation |
| `@xray-network/xray-cardano-lib-plutus/data` | Typed Plutus Data schemas and codecs |
| `@xray-network/xray-cardano-lib-plutus/uplc` | UPLC AST, Flat/text codecs, costs, and CEK evaluation |

The package is universal ESM, uses `Uint8Array` and Web Platform APIs, and is distributed under
the [MIT License](./LICENSE).
