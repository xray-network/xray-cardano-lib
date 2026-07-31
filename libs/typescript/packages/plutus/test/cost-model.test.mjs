import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import test from "node:test";

test("cost-model mapping tolerates short and extra tails deterministically", async () => {
  const [
    { builtinCost, makeBuiltinCostModel },
    { COST_MODEL_PARAMETER_NAMES },
  ] = await Promise.all([
    import("../dist/esm/uplc/cost-model.js"),
    import("../dist/esm/uplc/cost-model-data.js"),
  ]);
  assert.deepEqual(COST_MODEL_PARAMETER_NAMES.map((names) => names.length), [332, 332, 350]);
  const golden = await readFile(new URL(
    "../../../../../updates/providers/uplc/0001-uplc/artifacts/plutus/plutus-ledger-api/CostModel/Params/CostModelParams/costModelParamNames.golden.txt",
    import.meta.url,
  ), "utf8");
  assert.deepEqual(COST_MODEL_PARAMETER_NAMES[2], golden.trimEnd().split("\n"));

  const short = makeBuiltinCostModel(0, [7n], "A");
  const exact = makeBuiltinCostModel(
    0,
    Array.from({ length: 332 }, (_, index) => index === 0 ? 7n : 0x7fff_ffff_ffff_ffffn),
    "A",
  );
  assert.deepEqual(builtinCost(0, [[1n], [1n]], short), builtinCost(0, [[1n], [1n]], exact));

  const base = Array.from({ length: 332 }, () => 1n);
  const extra = makeBuiltinCostModel(0, [...base, 999_999n], "A");
  assert.deepEqual(
    builtinCost(0, [[1n], [1n]], extra),
    builtinCost(0, [[1n], [1n]], makeBuiltinCostModel(0, base, "A")),
  );
});
