import assert from "node:assert/strict";
import test from "node:test";
import { BigInteger } from "@xray-network/xray-cardano-lib-core";
import { PlutusData, PlutusDataList } from "@xray-network/xray-cardano-lib-chain";
import { parse_plutus_blueprint, validate_blueprint_value } from "@xray-network/xray-cardano-lib-plutus/blueprint";

const minimal = { preamble: { title: "Example", version: "1.0.0", plutusVersion: "v2" }, validators: [{ title: "spend", redeemer: { schema: { dataType: "integer", minimum: 0 } } }] };
test("CIP-57 blueprint parsing is bounded, immutable, and validates Data", () => {
  const blueprint = parse_plutus_blueprint(JSON.stringify(minimal)); assert.equal(blueprint.preamble.title, "Example"); assert.ok(Object.isFrozen(blueprint.validators));
  const good = PlutusData.new_integer(BigInteger.from_str("2")), bad = PlutusData.new_integer(BigInteger.from_str("-1"));
  assert.deepEqual(validate_blueprint_value({ dataType: "integer", minimum: 0, multipleOf: 2 }, good), []);
  assert.equal(validate_blueprint_value({ dataType: "integer", minimum: 0 }, bad)[0].code, "MINIMUM");
  assert.throws(() => parse_plutus_blueprint(JSON.stringify({ ...minimal, preamble: { ...minimal.preamble, plutusVersion: "v4" } })));
  assert.throws(() => parse_plutus_blueprint('{"preamble":{"title":"x","version":"1","plutusVersion":"v1"},"validators":[],"__proto__":{}}'));
});

test("CIP-57 resolves local recursive definitions and builtin parameter constants", () => {
  const blueprint = parse_plutus_blueprint(JSON.stringify({
    preamble: minimal.preamble,
    definitions: {
      tree: { anyOf: [
        { dataType: "integer" },
        { dataType: "list", items: { $ref: "#/definitions/tree" } },
      ] },
    },
    validators: [{ title: "tree", redeemer: { schema: { $ref: "#/definitions/tree" } } }],
  }));
  const schema = blueprint.validators[0].redeemer.schema;
  const nested = PlutusData.new_list(PlutusDataList.from([
    PlutusData.new_integer(BigInteger.from_str("1")),
  ]));
  assert.deepEqual(validate_blueprint_value(schema, nested), []);
  assert.deepEqual(validate_blueprint_value({ dataType: "#integer" }, { type: { kind: "integer" }, value: 4n }), []);
  assert.throws(() => parse_plutus_blueprint('{"preamble":{"title":"x","title":"y","version":"1","plutusVersion":"v1"},"validators":[]}'));
  assert.throws(() => parse_plutus_blueprint(JSON.stringify({ ...minimal, validators: [{ title: "x", redeemer: { schema: { $ref: "https://example.test/schema" } } }] })));
});
