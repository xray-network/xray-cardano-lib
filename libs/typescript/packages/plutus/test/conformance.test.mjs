import assert from "node:assert/strict";
import { createHash } from "node:crypto";
import { readFile } from "node:fs/promises";
import test from "node:test";

const corpusPath = new URL(
  "../../../../../.xray/updates/providers/uplc/0001-uplc/artifacts/conformance/corpus.json",
  import.meta.url,
);

test("all captured official UPLC evaluation and budget vectors pass", async () => {
  const [{ parseUplcText }, { evaluateProgram }] = await Promise.all([
    import("../dist/esm/uplc/text.js"),
    import("../dist/esm/uplc/machine.js"),
  ]);
  const bytes = await readFile(corpusPath);
  const corpus = JSON.parse(bytes.toString("utf8"));
  assert.equal(corpus.schemaVersion, 1);
  assert.equal(corpus.entries.length, 3_013);

  const files = new Map();
  for (const entry of corpus.entries) {
    const content = Buffer.from(entry.contentBase64, "base64");
    assert.equal(content.length, entry.size, `${entry.path}: size`);
    assert.equal(
      createHash("sha256").update(content).digest("hex"),
      entry.sha256,
      `${entry.path}: SHA-256`,
    );
    files.set(entry.path, content.toString("utf8"));
  }

  const failures = [];
  let vectors = 0;
  for (const [path, source] of files) {
    if (!path.endsWith(".uplc")) continue;
    vectors += 1;
    const expectedSource = files.get(`${path}.expected`);
    const budgetSource = files.get(`${path}.budget.expected`);
    assert.notEqual(expectedSource, undefined, `${path}: missing result`);
    assert.notEqual(budgetSource, undefined, `${path}: missing budget`);
    try {
      if (expectedSource.trim() === "parse error") {
        assert.throws(() => parseUplcText(source));
        continue;
      }
      const program = parseUplcText(source);
      if (expectedSource.trim() === "evaluation failure") {
        assert.throws(() => evaluateProgram(
          program,
          [],
          { cpu: 0x7fff_ffff_ffff_ffffn, memory: 0x7fff_ffff_ffff_ffffn },
        ));
        continue;
      }
      const expected = parseUplcText(expectedSource);
      const result = evaluateProgram(
        program,
        [],
        { cpu: 0x7fff_ffff_ffff_ffffn, memory: 0x7fff_ffff_ffff_ffffn },
      );
      assert.deepEqual(result.value, expected.term);
      const match = /\(\{cpu:\s*(\d+)\s*\|\s*mem:\s*(\d+)\}\)/m.exec(budgetSource);
      assert.notEqual(match, null, `${path}: invalid budget`);
      assert.deepEqual(result.budget, {
        cpu: BigInt(match[1]),
        memory: BigInt(match[2]),
      });
    } catch (error) {
      failures.push(`${path}: ${error?.stack ?? error}`);
      if (failures.length >= 20) break;
    }
  }
  assert.deepEqual(failures, []);
  assert.equal(vectors, 1_003);
});
