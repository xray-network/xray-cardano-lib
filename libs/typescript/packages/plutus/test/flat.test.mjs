import assert from "node:assert/strict";
import test from "node:test";

const loadFlat = () => import("../dist/esm/uplc/flat.js");

test("phase-two script envelopes retain only the V1/V2 CBOR remainder exception", async () => {
  const {
    decodeProgramEnvelope,
    decodeProgramEnvelopeCompatible,
    encodeProgramEnvelope,
  } = await loadFlat();
  const script = encodeProgramEnvelope({
    version: [1n, 0n, 0n],
    term: { kind: "constant", constant: { type: { kind: "unit" }, value: null } },
  });
  const withRemainder = Uint8Array.of(...script, 0);

  assert.throws(() => decodeProgramEnvelope(withRemainder), /trailing/iu);
  assert.doesNotThrow(() => decodeProgramEnvelopeCompatible(withRemainder, true));
  assert.throws(() => decodeProgramEnvelopeCompatible(withRemainder, false), /trailing/iu);
});

test("protocol-11 Flat universe and constructor bounds are enforced at decoding", async () => {
  const {
    decodeFlatProgram,
    encodeFlatProgram,
  } = await loadFlat();

  let type = { kind: "integer" };
  let value = 0n;
  for (let index = 0; index < 16; index += 1) {
    type = { kind: "list", item: type };
    value = [];
  }
  const wideUniverse = encodeFlatProgram({
    version: [1n, 0n, 0n],
    term: { kind: "constant", constant: { type, value } },
  });
  assert.doesNotThrow(() => decodeFlatProgram(wideUniverse));
  assert.throws(
    () => decodeFlatProgram(wideUniverse, { maxUniverseHeader: 32 }),
    /protocol limit/u,
  );

  const manyFields = encodeFlatProgram({
    version: [1n, 1n, 0n],
    term: {
      kind: "constr",
      tag: 0n,
      fields: Array.from(
        { length: 1_025 },
        () => ({ kind: "constant", constant: { type: { kind: "unit" }, value: null } }),
      ),
    },
  });
  assert.doesNotThrow(() => decodeFlatProgram(manyFields));
  assert.throws(
    () => decodeFlatProgram(manyFields, { maxConstrFields: 1_024 }),
    /term list is too long/u,
  );
});

test("the historical V1 Data decoder can retain an over-64-byte definite leaf", async () => {
  const { decodeFlatProgram } = await loadFlat();
  const cborData = Uint8Array.of(0x58, 0x41, ...new Uint8Array(65));
  const flat = Uint8Array.of(
    0x01, 0x00, 0x00,
    0x4c, 0x01,
    cborData.length,
    ...cborData,
    0x00,
    0x01,
  );

  assert.doesNotThrow(() => decodeFlatProgram(flat, { enforceDataWireLimit: false }));
  assert.throws(
    () => decodeFlatProgram(flat, { enforceDataWireLimit: true }),
    /64 bytes/u,
  );
});

test("Flat term encoding and decoding handle adversarial depth iteratively", async () => {
  const {
    decodeFlatProgram,
    encodeFlatProgram,
  } = await loadFlat();
  let term = { kind: "constant", constant: { type: { kind: "unit" }, value: null } };
  for (let index = 0; index < 20_000; index += 1) term = { kind: "delay", term };

  const decoded = decodeFlatProgram(encodeFlatProgram({ version: [1n, 0n, 0n], term }));
  let current = decoded.term;
  let depth = 0;
  while (current.kind === "delay") {
    depth += 1;
    current = current.term;
  }
  assert.equal(depth, 20_000);
  assert.equal(current.kind, "constant");
});
