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

test("CIP-57 generic, qualified and tuple definition keys resolve through all argument types", () => {
  // Original local schemas exercise opaque definition names, not a type-expression parser.
  const definitions = {
    Int: { dataType: "integer" },
    "cardano/address/StakeCredential": { dataType: "bytes", minLength: 1 },
    "Option<Int>": { title: "Option", dataType: "constructor", index: 1, fields: [{ $ref: "#/definitions/Int" }] },
    "Option<Option<Int>>": { dataType: "constructor", index: 1, fields: [{ $ref: "#/definitions/Option<Int>" }] },
    "Option<cardano/address/StakeCredential>": { dataType: "constructor", index: 1, fields: [{ $ref: "#/definitions/cardano~1address~1StakeCredential" }] },
    "Tuple<<Int,ByteArray>>": { dataType: "list", minItems: 2, maxItems: 2, items: [{ $ref: "#/definitions/Int" }, { dataType: "bytes" }] },
  };
  for (const [name, reference, good, bad] of [
    ["Option<Int>", "#/definitions/Option<Int>", "d87a8101", "d87a8141aa"],
    ["Option<Option<Int>>", "#/definitions/Option<Option<Int>>", "d87a81d87a8101", "d87a8101"],
    ["Option<cardano/address/StakeCredential>", "#/definitions/Option<cardano~1address~1StakeCredential>", "d87a8141aa", "d87a8101"],
    ["Tuple<<Int,ByteArray>>", "#/definitions/Tuple<<Int,ByteArray>>", "820141aa", "8241aa01"],
  ]) {
    const schema = { $ref: reference };
    const blueprint = parse_plutus_blueprint(JSON.stringify({
      preamble: minimal.preamble, definitions,
      validators: [{ title: "test", redeemer: { schema }, datum: { schema }, parameters: [{ schema }] }],
    }));
    assert.ok(Object.hasOwn(blueprint.definitions, name));
    assert.ok(Object.isFrozen(blueprint.definitions[name]));
    const validator = blueprint.validators[0];
    for (const argument of [validator.redeemer, validator.datum, validator.parameters[0]]) {
      assert.deepEqual(validate_blueprint_value(argument.schema, PlutusData.from_cbor_hex(good)), [], name);
      assert.notEqual(validate_blueprint_value(argument.schema, PlutusData.from_cbor_hex(bad)).length, 0, name);
    }
  }
});

test("CIP-57 titles do not change definition identity and pointer escapes decode once", () => {
  for (const title of [undefined, "Option", "Option<Int>", "Different display title"]) {
    const blueprint = parse_plutus_blueprint(JSON.stringify({
      preamble: minimal.preamble,
      definitions: {
        "Name~1Part/Type~0": { title, dataType: "integer" },
        "Name/Part/Type~0": { dataType: "bytes" },
      },
      validators: [{ title: "test", redeemer: { schema: { $ref: "#/definitions/Name~01Part~1Type~00" } } }],
    }));
    const schema = blueprint.validators[0].redeemer.schema;
    assert.deepEqual(validate_blueprint_value(schema, PlutusData.from_cbor_hex("01")), []);
    assert.notEqual(validate_blueprint_value(schema, PlutusData.from_cbor_hex("41aa")).length, 0);
    assert.equal(blueprint.definitions["Name~1Part/Type~0"].title, title);
  }
});

test("CIP-57 generic names do not relax reference or JSON safety", () => {
  for (const reference of ["#/definitions/Missing<Int>", "#/definitions/Display", "https://example.test/schema", "file:///schema", "#/definitions/constructor"]) {
    assert.throws(() => parse_plutus_blueprint(JSON.stringify({
      preamble: minimal.preamble,
      definitions: { "Option<Int>": { title: "Display", dataType: "integer" } },
      validators: [{ title: "test", redeemer: { schema: { $ref: reference } } }],
    })));
  }
  assert.throws(() => parse_plutus_blueprint(`{"preamble":${JSON.stringify(minimal.preamble)},"definitions":{"Option<Int>":{},"Option<Int>":{}},"validators":[]}`), /duplicate JSON object key/);
});
