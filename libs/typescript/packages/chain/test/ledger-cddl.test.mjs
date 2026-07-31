import assert from "node:assert/strict";
import test from "node:test";

import {
  encodeCbor,
} from "@xray-network/cardano-core";
import {
  AllegraTransactionBody,
} from "@xray-network/cardano-chain/allegra";
import {
  AlonzoTransactionBody,
} from "@xray-network/cardano-chain/alonzo";
import {
  BabbageTransactionBody,
} from "@xray-network/cardano-chain/babbage";
import {
  ByronBlock,
  ByronTxInRegular,
} from "@xray-network/cardano-chain/byron";
import {
  Certificate,
  ExUnits,
  GovAction,
  NetworkId,
  ProtocolParamUpdate,
  ProtocolVersion,
  TransactionBody,
  UnitInterval,
} from "@xray-network/cardano-chain/conway";
import {
  MaryTransactionBody,
} from "@xray-network/cardano-chain/mary";
import {
  MultiEraBlock,
} from "@xray-network/cardano-chain/multi-era";
import {
  ShelleyTransactionBody,
} from "@xray-network/cardano-chain/shelley";

const uint = (value) => ({ kind: "unsigned", value, encoding: { width: 0 } });
const bytes = (value) => ({
  kind: "bytes",
  value,
  encoding: { kind: "definite", width: 0 },
});
const array = (values) => ({
  kind: "array",
  values,
  encoding: { kind: "definite", width: 0 },
});
const map = (entries) => ({
  kind: "map",
  entries,
  encoding: { kind: "definite", width: 0 },
});
const tagged = (tag, value) => ({
  kind: "tag",
  tag,
  value,
  encoding: { width: 0 },
});
const fromNode = (Owner, node) => Owner.from_cbor_bytes(encodeCbor(node));

function minimalBody(extra = []) {
  return map([
    [uint(0n), array([])],
    [uint(1n), array([])],
    [uint(2n), uint(0n)],
    ...extra,
  ]);
}

test("historical transaction bodies enforce their era-specific mandatory fields and keys", () => {
  assert.throws(() => fromNode(ShelleyTransactionBody, uint(0n)));
  assert.throws(() => fromNode(ShelleyTransactionBody, map([])));
  assert.doesNotThrow(() => fromNode(
    ShelleyTransactionBody,
    minimalBody([[uint(3n), uint(0n)]]),
  ));

  for (const Owner of [
    AllegraTransactionBody,
    MaryTransactionBody,
    AlonzoTransactionBody,
    BabbageTransactionBody,
  ]) {
    assert.throws(() => fromNode(Owner, map([])), Owner.name);
    assert.throws(
      () => fromNode(Owner, minimalBody([[uint(255n), uint(0n)]])),
      Owner.name,
    );
    assert.doesNotThrow(() => fromNode(Owner, minimalBody()), Owner.name);
  }
});

test("Conway transaction, certificate, governance, and scalar bounds follow official CDDL", () => {
  assert.throws(() => fromNode(TransactionBody, map([])));
  assert.doesNotThrow(() => fromNode(TransactionBody, minimalBody()));
  assert.throws(() => fromNode(Certificate, array([])));
  assert.throws(() => fromNode(GovAction, array([])));
  assert.doesNotThrow(() => fromNode(GovAction, array([uint(6n)])));

  assert.throws(() => NetworkId.new(2n));
  assert.throws(() => ProtocolVersion.new(13n, 0n).to_cbor_bytes());
  assert.throws(() => fromNode(
    ProtocolParamUpdate,
    map([[uint(8n), uint(0xffff_ffff_ffff_ffffn)]]),
  ));
  assert.throws(() => UnitInterval.from_cbor_bytes(encodeCbor(
    tagged(30n, array([uint(2n), uint(1n)])),
  )));
  assert.throws(() => ExUnits.new(0x8000_0000_0000_0000n, 0n));
});

test("Byron wrappers reject wrong shapes and accept an official epoch-boundary structure", () => {
  assert.throws(() => ByronBlock.from_cbor_bytes(encodeCbor(uint(0n))));
  assert.throws(() => ByronTxInRegular.from_cbor_bytes(encodeCbor(array([uint(0n), bytes(new Uint8Array())]))));

  const emptyAttributes = map([]);
  const ebb = array([
    array([
      uint(0n),
      bytes(new Uint8Array(32)),
      bytes(new Uint8Array(32)),
      array([uint(0n), array([uint(0n)])]),
      array([emptyAttributes]),
    ]),
    array([]),
    array([emptyAttributes]),
  ]);
  const block = ByronBlock.from_cbor_bytes(encodeCbor(ebb));
  assert.deepEqual(block.to_cbor_bytes(), encodeCbor(ebb));
});

test("multi-era JSON cannot bypass era validation", () => {
  assert.throws(() => MultiEraBlock.from_json('{"Babbage":{}}'));
  assert.throws(() => MultiEraBlock.from_json('{"Conway":{}}'));
});
