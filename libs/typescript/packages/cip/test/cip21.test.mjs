import assert from "node:assert/strict";
import test from "node:test";
import { encodeCbor } from "@xray-network/xray-cardano-lib-core";
import { Transaction } from "@xray-network/xray-cardano-lib-chain/conway";
import { diagnose_cip21_transaction } from "@xray-network/xray-cardano-lib-cip/cip21";

const uint = (value) => ({ kind: "unsigned", value: BigInt(value), encoding: { width: 0 } });
const bytes = (value) => ({ kind: "bytes", value, encoding: { kind: "definite", width: 0 } });
const array = (values) => ({ kind: "array", values, encoding: { kind: "definite", width: 0 } });
const map = (entries) => ({ kind: "map", entries, encoding: { kind: "definite", width: 0 } });

test("CIP-21 reports stable advisory and Catalyst diagnostics over preserved CBOR", () => {
  const address = new Uint8Array(29); address[0] = 0x61;
  const input = array([bytes(new Uint8Array(32)), uint(0)]);
  const body = map([
    [uint(0), { kind: "tag", tag: 258n, value: array([input]), encoding: { width: 4 } }],
    [uint(1), array([array([bytes(address), uint(1)])])],
    [uint(2), uint(1)],
  ]);
  const transaction = Transaction.from_cbor_bytes(encodeCbor(array([
    body, map([]), { kind: "boolean", value: true }, { kind: "null" },
  ])));
  const report = diagnose_cip21_transaction(transaction);
  assert.ok(report.some((item) => item.code === "NON_CANONICAL_INTEGER"));
  const catalyst = diagnose_cip21_transaction(transaction, { auxiliaryDataMode: "catalyst-registration" });
  assert.ok(catalyst.some((item) => item.code === "INVALID_CATALYST_AUXILIARY_DATA"));
  assert.deepEqual([...report], [...report].sort((left, right) => left.path.localeCompare(right.path) || left.code.localeCompare(right.code)));
});
