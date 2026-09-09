import assert from "node:assert/strict";
import { createHash } from "node:crypto";
import test from "node:test";
import { encodeFlatProgram } from "../dist/esm/uplc/flat.js";
import { readConformanceCorpus, runConformanceCorpus } from "./support/conformance.mjs";

// Original local transport fixtures, not copied upstream conformance evidence.
const integerText = "(program 1.0.0 (con integer 7))";
const bytesText = "(program 1.0.0 (con bytestring #ff80))";
const errorText = "(program 1.0.0 (error))";
const constantBudget = "({cpu: 16100 | mem: 200})";
const bytesFlat = encodeFlatProgram({
  version: [1n, 0n, 0n],
  term: { kind: "constant", constant: { type: { kind: "bytes" }, value: Uint8Array.of(255, 128) } },
});
const errorFlat = encodeFlatProgram({ version: [1n, 0n, 0n], term: { kind: "error" } });

function document(files) {
  return {
    schemaVersion: 1,
    entries: files.map(([path, value]) => {
      const bytes = Buffer.from(value);
      return { path, size: bytes.length, sha256: createHash("sha256").update(bytes).digest("hex"),
        contentBase64: bytes.toString("base64") };
    }).sort((a, b) => Buffer.compare(Buffer.from(a.path), Buffer.from(b.path))),
  };
}

const serialize = (corpus) => Buffer.from(JSON.stringify(corpus));
const historical = () => [
  ["local/seven.uplc", integerText],
  ["local/seven.uplc.expected", integerText],
  ["local/seven.uplc.budget.expected", constantBudget],
];
const run = (files, caseCount = 1, auxiliaryPaths = []) => runConformanceCorpus(serialize(document(files)), {
  entryCount: files.length, caseCount, auxiliaryPaths,
});

test("miniature historical text layout has one explicit successful disposition", () => {
  assert.deepEqual(run(historical()), {
    entryCount: 3, dispositions: [{ path: "local/seven.uplc", outcome: "success" }],
  });
});

test("miniature text and binary Flat siblings share a modern budget without UTF-8 conversion", () => {
  const files = [
    ["local/bytes.uplc", bytesText], ["local/bytes.uplc.expected", bytesText],
    ["local/bytes.flat", bytesFlat], ["local/bytes.flat.expected", bytesFlat],
    ["local/bytes.budget.expected", constantBudget], ["local/README.md", "Local transport cases."],
  ];
  assert.throws(() => new TextDecoder("utf-8", { fatal: true }).decode(bytesFlat));
  const inventory = { entryCount: 6, caseCount: 2, auxiliaryPaths: ["local/README.md"] };
  const decoded = readConformanceCorpus(serialize(document(files)), inventory);
  assert.deepEqual(decoded.find(({ path }) => path.endsWith(".flat")).source, bytesFlat);
  assert.deepEqual(run(files, 2, inventory.auxiliaryPaths).dispositions, [
    { path: "local/bytes.flat", outcome: "success" },
    { path: "local/bytes.uplc", outcome: "success" },
  ]);
});

test("miniature failure controls distinguish codec and evaluator stages for both formats", () => {
  const files = [];
  for (const [name, source, marker] of [
    ["text.uplc", "broken", "parse error"],
    ["binary.flat", Uint8Array.of(255), "parse/decode error"],
    ["text-error.uplc", errorText, "evaluation failure"],
    ["binary-error.flat", errorFlat, "evaluation failure"],
  ]) {
    files.push([name, source], [`${name}.expected`, marker], [`${name}.budget.expected`, `${marker}\n`]);
  }
  const { dispositions } = run(files, 4);
  assert.equal(dispositions.length, 4);
  assert.equal(dispositions.filter(({ outcome }) => outcome === "codec failure").length, 2);
  assert.equal(dispositions.filter(({ outcome }) => outcome === "evaluation failure").length, 2);
});

test("miniature failures cannot be satisfied by errors from a different stage", () => {
  for (const [path, valid, invalid] of [["x.uplc", errorText, "broken"], ["x.flat", errorFlat, Uint8Array.of(255)]]) {
    assert.throws(() => run([
      [path, valid], [`${path}.expected`, "parse/decode error"], [`${path}.budget.expected`, "parse/decode error"],
    ]), /expected codec failure/);
    assert.throws(() => run([
      [path, invalid], [`${path}.expected`, "evaluation failure"], [`${path}.budget.expected`, "evaluation failure"],
    ]));
  }
  assert.throws(() => run([
    ["x.uplc", integerText], ["x.uplc.expected", "evaluation failure"], ["x.uplc.budget.expected", "evaluation failure"],
  ]), /expected evaluation failure/);
  assert.throws(() => run([
    ["x.uplc", "broken"], ["x.uplc.expected", "parse error"], ["x.uplc.budget.expected", constantBudget],
  ]), /failure budget marker/);
});

test("miniature success requires valid semantic results and exact budgets", () => {
  for (const [expected, budget, error] of [
    [bytesText, constantBudget, /result/],
    [integerText, "({cpu: 16101 | mem: 200})", /budget/],
    [integerText, "garbage ({cpu: 16100 | mem: 200})", /invalid budget/],
    ["broken", constantBudget, undefined],
  ]) {
    assert.throws(() => run([
      ["x.uplc", integerText], ["x.uplc.expected", expected], ["x.budget.expected", budget],
    ]), error);
  }
  for (const expected of [Uint8Array.of(255), errorFlat]) {
    assert.throws(() => run([
      ["x.flat", bytesFlat], ["x.flat.expected", expected], ["x.budget.expected", constantBudget],
    ]));
  }
});

test("miniature inventories reject missing, ambiguous, orphaned and unknown files", () => {
  assert.throws(() => run(historical().filter(([path]) => !path.endsWith(".uplc.expected"))), /missing result/);
  assert.throws(() => run(historical().filter(([path]) => !path.endsWith("budget.expected"))), /missing or ambiguous budget/);
  for (const budget of [constantBudget, "({cpu: 1 | mem: 1})"]) {
    assert.throws(() => run([...historical(), ["local/seven.budget.expected", budget]]), /ambiguous budget/);
  }
  for (const path of ["orphan.uplc.expected", "orphan.budget.expected", "new-format.bin", "README.md"]) {
    assert.throws(() => run([...historical(), [path, "unknown"]]), /unaccounted corpus files/);
  }
  assert.throws(() => run(historical(), 1, ["missing.md"]), /invalid auxiliary path/);
  assert.throws(() => run(historical(), 1, ["local/seven.uplc"]), /invalid auxiliary path/);
  assert.throws(() => run([...historical(), ["README.md", "local"]], 1, ["README.md", "README.md"]), /duplicate auxiliary path/);
});

test("miniature inventory integrity fails before case execution", () => {
  const mutations = [
    [(c) => { c.entries[0].contentBase64 += "\n"; }, /base64/],
    [(c) => { c.entries[0].contentBase64 = "Zg"; }, /base64/],
    [(c) => { c.entries[0].size += 1; }, /size/],
    [(c) => { c.entries[0].size = -1; }, /size/],
    [(c) => { c.entries[0].size = 16 * 1024 * 1024 + 1; }, /size/],
    [(c) => { c.entries[0].sha256 = "0".repeat(64); }, /SHA-256/],
    [(c) => { c.entries[0].sha256 = "invalid"; }, /SHA-256/],
    [(c) => { c.entries[1] = { ...c.entries[0] }; }, /duplicate path/],
    [(c) => { c.entries.reverse(); }, /unsorted inventory/],
    [(c) => { c.schemaVersion = 2; }, /schema/],
  ];
  for (const [mutate, error] of mutations) {
    const corpus = document(historical());
    mutate(corpus);
    assert.throws(() => runConformanceCorpus(serialize(corpus), { entryCount: 3, caseCount: 1 }), error);
  }
  for (const path of ["", "/absolute", "../escape", "local/../escape", "local/./x", "local//x", "a\\b", "C:/escape", "x\u0000y"]) {
    const corpus = document(historical());
    corpus.entries[0].path = path;
    assert.throws(() => runConformanceCorpus(serialize(corpus), { entryCount: 3, caseCount: 1 }), /unsafe corpus path/);
  }
});

test("miniature inventory and case counts must match an explicit nonempty selection", () => {
  const bytes = serialize(document(historical()));
  for (const [entryCount, caseCount] of [[4, 1], [3, 2], [0, 1], [3, 0], [undefined, 1], [3, undefined]]) {
    assert.throws(() => runConformanceCorpus(bytes, { entryCount, caseCount }), /count/);
  }
});
