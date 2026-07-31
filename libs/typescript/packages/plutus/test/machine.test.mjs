import assert from "node:assert/strict";
import test from "node:test";

const maximum = { cpu: 0x7fff_ffff_ffff_ffffn, memory: 0x7fff_ffff_ffff_ffffn };
const constant = (type, value) => ({ kind: "constant", constant: { type, value } });
const apply = (function_, argument) => ({ kind: "apply", function: function_, argument });
const apply2 = (tag, left, right) => apply(apply({ kind: "builtin", tag }, left), right);

test("CEK evaluation preserves Trace ordering and charges before returning", async () => {
  const { evaluateProgram } = await import("../dist/esm/uplc/machine.js");
  const trace = (message, result) => apply(
    apply(
      { kind: "force", term: { kind: "builtin", tag: 28 } },
      constant({ kind: "string" }, message),
    ),
    result,
  );
  const result = evaluateProgram({
    version: [1n, 0n, 0n],
    term: trace("outer", trace("inner", constant({ kind: "unit" }, null))),
  }, [], maximum);

  assert.deepEqual(result.logs, ["inner", "outer"]);
  assert.equal(result.isUnit, true);
  assert.equal(result.budget.cpu > 0n, true);
  assert.equal(result.budget.memory > 0n, true);
});

test("protocol-11 D/E semantics bound byte-string shifts without leaking into A-C", async () => {
  const { evaluateProgram } = await import("../dist/esm/uplc/machine.js");
  const program = {
    version: [1n, 1n, 0n],
    term: apply2(
      82,
      constant({ kind: "bytes" }, Uint8Array.of(1)),
      constant({ kind: "integer" }, 0x8000_0000_0000_0000n),
    ),
  };

  assert.equal(evaluateProgram(program, [], maximum, undefined, "A").isUnit, false);
  assert.throws(
    () => evaluateProgram(program, [], maximum, undefined, "D"),
    /signed 64-bit bounds/u,
  );
  assert.throws(
    () => evaluateProgram(program, [], maximum, undefined, "E"),
    /signed 64-bit bounds/u,
  );
});

test("D/E string builtin costing uses UTF-8 byte length", async () => {
  const { evaluateProgram } = await import("../dist/esm/uplc/machine.js");
  const program = {
    version: [1n, 0n, 0n],
    term: apply2(
      22,
      constant({ kind: "string" }, "é"),
      constant({ kind: "string" }, ""),
    ),
  };

  const legacy = evaluateProgram(program, [], maximum, undefined, "A");
  const bounded = evaluateProgram(program, [], maximum, undefined, "D");
  assert.notEqual(legacy.budget.cpu, bounded.budget.cpu);
});

test("CEK evaluation handles adversarially deep application trees iteratively", async () => {
  const { evaluateProgram } = await import("../dist/esm/uplc/machine.js");
  const identity = { kind: "lambda", body: { kind: "var", index: 1n } };
  let term = constant({ kind: "unit" }, null);
  for (let index = 0; index < 20_000; index += 1) term = apply(identity, term);

  const result = evaluateProgram({ version: [1n, 0n, 0n], term }, [], maximum);
  assert.equal(result.isUnit, true);
});

test("CEK discharge handles adversarially deep returned values iteratively", async () => {
  const { evaluateProgram } = await import("../dist/esm/uplc/machine.js");
  let term = constant({ kind: "unit" }, null);
  for (let index = 0; index < 20_000; index += 1) term = { kind: "delay", term };

  let discharged = evaluateProgram({ version: [1n, 0n, 0n], term }, [], maximum).value;
  let depth = 0;
  while (discharged.kind === "delay") {
    depth += 1;
    discharged = discharged.term;
  }
  assert.equal(depth, 20_000);
});
