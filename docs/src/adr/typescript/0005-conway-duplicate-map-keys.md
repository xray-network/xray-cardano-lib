# ADR 0005: Reject duplicate keys in Conway transaction maps

- Status: Proposed
- Date: 2026-11-03

## Context

ADR 0001 preserves duplicate map entries so decoded values re-encode byte-for-byte. For Conway
transactions that preservation is unsafe at signing boundaries. A transaction body such as
`{0: inputs, 1: outputsA, 2: feeA, 1: outputsB, 2: feeB}` decodes losslessly and hashes over the
original bytes, but consumers disagree about its meaning: first-match readers (including the
XRAY JS transaction inspector) report `outputsA`, while last-wins readers act on `outputsB`. A
wallet that shows one interpretation and signs the bytes therefore signs something the user did not
review.

The Conway ledger is expected to reject duplicate keys in these maps, but this library should not
depend on that: a transaction whose meaning depends on the reader is unsafe to inspect or sign
regardless of whether a particular node would accept it.

## Decision

- `fixedMap` and `mapOf` accept an opt-in `uniqueKeys` flag. Key identity is semantic: keys are
  compared by their canonical encoding, so the same value written with different head widths or
  container forms is a duplicate.
- `fixedMap` with `uniqueKeys` also rejects repeated unknown field keys.
- Conway validators enable the flag for transaction bodies, post-Alonzo outputs, multi-asset values
  and mint (both policy and asset-name levels), withdrawals, voting procedures, map-form redeemers,
  witness sets, and Conway-format auxiliary data.
- Plutus data, transaction metadata, and pre-Conway eras keep lossless duplicate preservation, as do
  the top-level transaction bodies and witness sets of historical Conway blocks.

## Consequences

- `Transaction`, `TransactionBody`, `TransactionWitnessSet`, `Withdrawals`, and the other listed
  Conway models throw `TypeError` on duplicate keys instead of decoding them.
- Lossless preservation still holds for every accepted value; only ambiguous inputs are refused.
- Downstream inspectors should still fail closed on duplicate fields rather than rely solely on this
  validator.
