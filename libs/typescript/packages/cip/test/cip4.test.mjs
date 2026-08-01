import assert from "node:assert/strict";
import test from "node:test";

import { CIP4 } from "@xray-network/xray-cardano-lib-cip/cip4";

const publicKeyHash = "13559f50b21a7229275ea7586e5b5a504c8ed1d9ddf17464c413f0ba";

test("CIP4 calculates the wallet checksum from the canonical public-key hash", () => {
  assert.deepEqual(CIP4.calculateChecksum(publicKeyHash), {
    checksumId: "XPPX-4012",
    checksumImage:
      "5751762ac8c0e9005efc68e624d3323e917109b1ab77b484baf2c256cb57abab" +
      "ca6b0879cbe753ed403b3b22971f81e1a4c6c7f4dd02977be41c41050a57f0f2",
  });
});

test("CIP4 rejects noncanonical public-key-hash inputs", () => {
  for (const value of [
    publicKeyHash.toUpperCase(),
    publicKeyHash.slice(2),
    `${publicKeyHash}00`,
    `${"0".repeat(55)}g`,
    "xpub1invalid",
    "",
  ]) {
    assert.throws(() => CIP4.calculateChecksum(value), /28-byte lowercase hexadecimal/u);
  }
});
