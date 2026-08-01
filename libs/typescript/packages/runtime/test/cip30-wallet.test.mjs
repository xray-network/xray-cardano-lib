import assert from "node:assert/strict";
import test from "node:test";

import {
  Address,
  TransactionHash,
  TransactionInput,
  TransactionOutputBuilder,
  TransactionUnspentOutput,
  TransactionWitnessSet,
  Value,
} from "@xray-network/xray-cardano-lib";
import { bytesToHex, hexToBytes } from "@xray-network/xray-cardano-lib-core";

async function connectWallet(provider) {
  const wallet = await provider.enable();
  const rawUtxos = await wallet.getUtxos();
  const utxos = (rawUtxos ?? []).map((value) => TransactionUnspentOutput.from_cbor_hex(value));
  const changeAddress = Address.from_raw_bytes(hexToBytes(await wallet.getChangeAddress()));
  return { wallet, utxos, changeAddress };
}

test("a mocked CIP-30 wallet interoperates through hex-only boundaries", async () => {
  const address = Address.from_raw_bytes(Uint8Array.from([0x60, ...new Uint8Array(28).fill(0x33)]));
  const input = TransactionInput.new(TransactionHash.from_raw_bytes(new Uint8Array(32).fill(0x44)), 7n);
  const output = TransactionOutputBuilder.new()
    .with_address(address)
    .next()
    .with_value(Value.from_coin(5_000_000n))
    .build()
    .output();
  const utxo = TransactionUnspentOutput.new(input, output);
  const witness = TransactionWitnessSet.new();
  const calls = [];
  const provider = {
    async enable() {
      calls.push("enable");
      return {
        async getUtxos() { calls.push("getUtxos"); return [utxo.to_cbor_hex()]; },
        async getChangeAddress() { calls.push("getChangeAddress"); return bytesToHex(address.to_raw_bytes()); },
        async signTx(transaction, partialSign) {
          calls.push(["signTx", transaction, partialSign]);
          return witness.to_cbor_hex();
        },
      };
    },
  };

  const connected = await connectWallet(provider);
  assert.deepEqual(calls, ["enable", "getUtxos", "getChangeAddress"]);
  assert.equal(connected.utxos.length, 1);
  assert.equal(connected.utxos[0].input().index(), 7n);
  assert.equal(connected.utxos[0].output().amount().coin(), 5_000_000n);
  assert.deepEqual(connected.changeAddress.to_raw_bytes(), address.to_raw_bytes());

  const signed = await connected.wallet.signTx("a0", true);
  assert.deepEqual(TransactionWitnessSet.from_cbor_hex(signed).to_cbor_bytes(), witness.to_cbor_bytes());
  assert.deepEqual(calls.at(-1), ["signTx", "a0", true]);
});

test("the mocked CIP-30 boundary rejects malformed wallet data", async () => {
  const provider = {
    async enable() {
      return {
        async getUtxos() { return ["ff"]; },
        async getChangeAddress() { return "00"; },
      };
    },
  };
  await assert.rejects(() => connectWallet(provider), /CBOR|transaction unspent output/u);
});
