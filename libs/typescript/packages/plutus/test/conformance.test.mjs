import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import test from "node:test";
import { runConformanceCorpus } from "./support/conformance.mjs";

const corpusPath = new URL(
  "../../../../../.agents/spectre/providers/uplc/0001/artifacts/conformance/corpus.json",
  import.meta.url,
);

test("all captured official UPLC evaluation and budget vectors pass", async () => {
  const result = runConformanceCorpus(await readFile(corpusPath), {
    entryCount: 3_013,
    caseCount: 1_003,
    auxiliaryPaths: [
      "uplc/evaluation/builtin/interleaving/README.md",
      "uplc/evaluation/builtin/semantics/README.md",
      "uplc/evaluation/builtin/semantics/bls12_381-cardano-crypto-tests/README.md",
      "uplc/evaluation/builtin/semantics/verifyEcdsaSecp256k1Signature/README.md",
    ],
  });
  assert.equal(result.dispositions.length, 1_003);
});
