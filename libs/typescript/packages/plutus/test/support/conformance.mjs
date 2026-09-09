import assert from "node:assert/strict";
import { createHash } from "node:crypto";
import { decodeFlatProgram } from "../../dist/esm/uplc/flat.js";
import { evaluateProgram } from "../../dist/esm/uplc/machine.js";
import { parseUplcText } from "../../dist/esm/uplc/text.js";

const maximum = { cpu: 0x7fff_ffff_ffff_ffffn, memory: 0x7fff_ffff_ffff_ffffn };
const maxEntrySize = 16 * 1024 * 1024;
const textDecoder = new TextDecoder("utf-8", { fatal: true });

// Inventory is selected explicitly by each caller, never inferred from a corpus.
export function readConformanceCorpus(bytes, { entryCount, caseCount, auxiliaryPaths = [] }) {
  assert.ok(Number.isSafeInteger(entryCount) && entryCount > 0, "invalid entry count");
  assert.ok(Number.isSafeInteger(caseCount) && caseCount > 0, "invalid case count");
  const corpus = JSON.parse(textDecoder.decode(bytes));
  assert.equal(corpus.schemaVersion, 1, "unsupported corpus schema");
  assert.ok(Array.isArray(corpus.entries), "missing corpus entries");
  assert.equal(corpus.entries.length, entryCount, "entry count");
  const verified = [];
  const paths = new Set();
  let previous;
  for (const entry of corpus.entries) {
    const path = entry.path;
    assert.ok(typeof path === "string" && path.length > 0
      && !/[\\\u0000-\u001f\u007f]/.test(path) && !/^[a-z]:/i.test(path)
      && path.split("/").every((part) => part !== "" && part !== "." && part !== ".."),
    "unsafe corpus path");
    assert.ok(!paths.has(path), `${path}: duplicate path`);
    paths.add(path);
    assert.ok(previous === undefined || Buffer.compare(Buffer.from(previous), Buffer.from(path)) < 0,
      `${path}: unsorted inventory`);
    previous = path;
    assert.ok(Number.isSafeInteger(entry.size) && entry.size >= 0 && entry.size <= maxEntrySize,
      `${path}: invalid size`);
    assert.ok(typeof entry.contentBase64 === "string"
      && entry.contentBase64.length <= 4 * Math.ceil(maxEntrySize / 3), `${path}: invalid base64`);
    const content = Buffer.from(entry.contentBase64, "base64");
    assert.equal(content.toString("base64"), entry.contentBase64, `${path}: noncanonical base64`);
    assert.equal(content.length, entry.size, `${path}: size`);
    assert.match(entry.sha256, /^[0-9a-f]{64}$/, `${path}: invalid SHA-256`);
    assert.equal(createHash("sha256").update(content).digest("hex"), entry.sha256, `${path}: SHA-256`);
    verified.push([path, new Uint8Array(content)]);
  }
  const files = new Map(verified);
  const consumed = new Set(auxiliaryPaths);
  assert.equal(consumed.size, auxiliaryPaths.length, "duplicate auxiliary path");
  for (const path of auxiliaryPaths) {
    assert.ok(typeof path === "string" && path.endsWith(".md") && files.has(path),
      `${path}: invalid auxiliary path`);
  }
  const cases = [];
  for (const [path, source] of files) {
    if (!path.endsWith(".uplc") && !path.endsWith(".flat")) continue;
    const resultPath = `${path}.expected`;
    assert.ok(files.has(resultPath), `${path}: missing result`);
    const budgetPaths = [`${path}.budget.expected`, `${path.slice(0, -5)}.budget.expected`]
      .filter((candidate) => files.has(candidate));
    assert.equal(budgetPaths.length, 1, `${path}: missing or ambiguous budget`);
    consumed.add(path);
    consumed.add(resultPath);
    consumed.add(budgetPaths[0]);
    cases.push({ path, source, expected: files.get(resultPath), budget: files.get(budgetPaths[0]) });
  }
  assert.equal(cases.length, caseCount, "case count");
  assert.deepEqual([...files.keys()].filter((path) => !consumed.has(path)), [], "unaccounted corpus files");
  return cases;
}

function failureMarker(bytes) {
  // Only short ASCII controls are text in an otherwise binary Flat result file.
  if (bytes.length > 64 || bytes.some((byte) => byte > 126 || (byte < 32 && ![9, 10, 13].includes(byte)))) {
    return undefined;
  }
  const value = String.fromCharCode(...bytes).trim();
  if (value === "parse error" || value === "parse/decode error") return "codec failure";
  if (value === "evaluation failure") return value;
  return undefined;
}

export function runConformanceCorpus(bytes, inventory) {
  const cases = readConformanceCorpus(bytes, inventory);
  const dispositions = [];
  for (const { path, source, expected, budget } of cases) {
    const decode = path.endsWith(".flat")
      ? decodeFlatProgram
      : (content) => parseUplcText(textDecoder.decode(content));
    const outcome = failureMarker(expected);
    if (outcome !== undefined) {
      assert.equal(failureMarker(budget), outcome, `${path}: failure budget marker`);
      if (outcome === "codec failure") {
        assert.throws(() => decode(source), `${path}: expected codec failure`);
      } else {
        const program = decode(source);
        assert.throws(() => evaluateProgram(program, [], maximum), `${path}: expected evaluation failure`);
      }
    } else {
      const program = decode(source);
      const expectedProgram = decode(expected);
      const match = /^\(\{cpu:\s*(\d+)\s*\|\s*mem:\s*(\d+)\}\)$/.exec(textDecoder.decode(budget).trim());
      assert.notEqual(match, null, `${path}: invalid budget`);
      const result = evaluateProgram(program, [], maximum);
      assert.deepEqual(result.value, expectedProgram.term, `${path}: result`);
      assert.deepEqual(result.budget, { cpu: BigInt(match[1]), memory: BigInt(match[2]) }, `${path}: budget`);
    }
    dispositions.push({ path, outcome: outcome ?? "success" });
  }
  return { entryCount: inventory.entryCount, dispositions };
}
