import assert from "node:assert/strict";
import test from "node:test";
import { readFile } from "node:fs/promises";
import {
  parseByronGenesis,
} from "../dist/esm/era/byron/genesis.js";
import {
  parseShelleyGenesis,
} from "../dist/esm/era/shelley/genesis.js";

const inventory = JSON.parse(
  await readFile(
    new URL(
      "../../../../../.xray/updates/providers/cardano-multiplatform-lib/0001-cardano-multiplatform-lib/artifacts/test-vectors/manifest.json",
      import.meta.url,
    ),
    "utf8",
  ),
);
const fixtureRoot = new URL(
  "../../../../../.xray/updates/providers/cardano-multiplatform-lib/0001-cardano-multiplatform-lib/artifacts/test-vectors/",
  import.meta.url,
);
const byronFixtures = inventory.fixtures.genesisJson.filter((fixture) =>
  fixture.path.startsWith("genesis/byron/")
);
const shelleyFixtures = inventory.fixtures.genesisJson.filter((fixture) =>
  fixture.path.startsWith("genesis/shelley/")
);

async function fixtureText(fixture) {
  return readFile(new URL(fixture.path, fixtureRoot), "utf8");
}

test("parse_test_genesis_files validates all four Byron vectors", async () => {
  assert.equal(byronFixtures.length, 4);
  const parsed = [];
  for (const fixture of byronFixtures) parsed.push(parseByronGenesis(await fixtureText(fixture)));
  const federal = parsed.find((genesis) => genesis.protocolMagic.to_int() === 633343913);
  assert.equal(federal.epochStabilityDepth, 2160);
  assert.equal(federal.startTime, 1506450213);
  assert.equal(federal.slotDurationMilliseconds, 20000);
  assert.equal(federal.feePolicy.coefficient(), 43946000000n);
  assert.equal(federal.feePolicy.constant(), 155381000000000n);
  assert.equal(federal.avvmDistribution.get("-0BJDi-gauylk4LptQTgjMeo7kY9lTCbZv12vwOSTZk="), 9999300000000n);
  assert.equal(parsed.some((genesis) => genesis.nonAvvmBalances.get("2cWKMJemoBaheSTiK9XEtQDf47Z3My8jwN25o5jjm7s7jaXin2nothhWQrTDd8m433M8K") === 5428571428571429n), true);
});

test("parse_test_genesis_files and parse_test_genesis_yaci_files validate Shelley data", async () => {
  assert.equal(shelleyFixtures.length, 2);
  for (const fixture of shelleyFixtures) {
    const name = fixture.path.split("/").at(-1);
    const genesis = parseShelleyGenesis(await fixtureText(fixture));
    assert.equal(genesis.epochLength, name === "test.json" ? 432000n : 600n);
    assert.equal(genesis.networkId, 0);
    assert.equal(genesis.networkMagic, name === "test.json" ? 764824073 : 42);
    if (name === "test.json") assert.equal(genesis.initialFunds.get("605276322ac7882434173dcc6441905f6737689bd309b68ad8b3614fd8"), 3000000000000000n);
  }
});
