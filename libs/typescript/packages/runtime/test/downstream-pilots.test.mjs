import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";

import * as cardano from "@xray-network/xray-cardano-lib";
import { bytesToHex, hexToBytes } from "@xray-network/xray-cardano-lib-core";
import { MultiEraBlock } from "@xray-network/xray-cardano-lib-chain/multi-era";

const privateKey = cardano.PrivateKey.from_normal_bytes(
  Uint8Array.from({ length: 32 }, (_, index) => index + 1),
);
const address = cardano.EnterpriseAddress.new(
  0,
  cardano.Credential.new_pub_key(privateKey.to_public().hash()),
).to_address();

function sourceUtxo(index = 0) {
  const hash = new Uint8Array(32); hash[31] = index;
  const input = cardano.TransactionInput.new(cardano.TransactionHash.from_raw_bytes(hash), BigInt(index));
  const output = cardano.TransactionOutputBuilder.new().with_address(address)
    .next().with_value(cardano.Value.from_coin(5_000_000n)).build().output();
  return cardano.TransactionUnspentOutput.new(input, output);
}

test("wallet pilot crosses only CIP-30 hex boundaries", async () => {
  const utxo = sourceUtxo(1);
  const witnesses = cardano.TransactionWitnessSet.new();
  const provider = {
    async enable() {
      return {
        async getUtxos() { return [utxo.to_cbor_hex()]; },
        async getChangeAddress() { return bytesToHex(address.to_raw_bytes()); },
        async signTx() { return witnesses.to_cbor_hex(); },
      };
    },
  };
  const wallet = await provider.enable();
  const decoded = (await wallet.getUtxos()).map((value) => cardano.TransactionUnspentOutput.from_cbor_hex(value));
  assert.equal(decoded[0].output().amount().coin(), 5_000_000n);
  assert.deepEqual(cardano.Address.from_raw_bytes(hexToBytes(await wallet.getChangeAddress())).to_raw_bytes(), address.to_raw_bytes());
  assert.deepEqual(cardano.TransactionWitnessSet.from_cbor_hex(await wallet.signTx("a0", true)).to_cbor_bytes(), witnesses.to_cbor_bytes());
});

test("dApp pilot constructs a balanced payment body", () => {
  const configuration = cardano.TransactionBuilderConfigBuilder.new()
    .fee_algo(cardano.LinearFee.new(1n, 10n, 0n))
    .pool_deposit(500n).key_deposit(100n)
    .max_value_size(5000).max_tx_size(16_384).coins_per_utxo_byte(1n)
    .ex_unit_prices(cardano.ExUnitPrices.new(cardano.Rational.new(1n, 10n), cardano.Rational.new(1n, 10n)))
    .collateral_percentage(150).max_collateral_inputs(3).build();
  const source = sourceUtxo(2);
  const builder = cardano.TransactionBuilder.new(configuration);
  builder.add_input(cardano.SingleInputBuilder.new(source.input(), source.output()).payment_key());
  builder.add_output(cardano.TransactionOutputBuilder.new().with_address(address)
    .next().with_value(cardano.Value.from_coin(2_000_000n)).build());
  const signed = builder.build(cardano.ChangeSelectionAlgo.Default, address);
  signed.add_vkey(cardano.make_vkey_witness(cardano.hash_transaction(signed.body()), privateKey));
  assert.ok(signed.build_checked().to_cbor_bytes().length > 0);
  assert.equal(builder.get_total_input().coin(), builder.get_total_output().coin() + builder.get_fee_if_set());
});

test("indexer pilot decodes and preserves one block from every era family", async () => {
  const inventory = JSON.parse(
    await readFile(
      new URL(
        "../../../../../updates/providers/cardano-multiplatform-lib/0001-cardano-multiplatform-lib/artifacts/test-vectors/manifest.json",
        import.meta.url,
      ),
      "utf8",
    ),
  );
  const fixtureRoot = fileURLToPath(new URL(
    "../../../../../updates/providers/cardano-multiplatform-lib/0001-cardano-multiplatform-lib/artifacts/test-vectors/",
    import.meta.url,
  ));
  const seen = new Set();
  for (const fixture of inventory.fixtures.goldenBlocks) {
    if (fixture.expected !== "byte-exact-round-trip" || seen.has(fixture.era)) continue;
    let bytes = await readFile(path.resolve(fixtureRoot, fixture.path));
    if (bytes.every((byte) => /[0-9a-f\s]/iu.test(String.fromCharCode(byte)))) bytes = Buffer.from(bytes.toString().trim(), "hex");
    const block = MultiEraBlock.from_explicit_network_cbor_bytes(bytes);
    assert.deepEqual(block.to_explicit_network_cbor_bytes(), bytes);
    assert.equal(typeof block.header().slot(), "bigint");
    seen.add(fixture.era);
  }
  assert.deepEqual([...seen].sort(), ["allegra", "alonzo", "babbage", "byron-ebb", "byron-main", "conway", "mary", "shelley"]);
});
