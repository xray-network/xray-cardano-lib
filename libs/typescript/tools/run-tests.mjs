import { spawnSync } from "node:child_process";
import { readdir } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

const typescriptRoot = fileURLToPath(new URL("../", import.meta.url));
const packageNames = ["core", "crypto", "chain", "cip", "plutus", "runtime"];

async function testFilesBelow(directory) {
  const files = [];
  for (const entry of await readdir(directory, { withFileTypes: true })) {
    const entryPath = path.join(directory, entry.name);
    if (entry.isDirectory()) files.push(...await testFilesBelow(entryPath));
    else if (entry.isFile() && entry.name.endsWith(".test.mjs")) files.push(entryPath);
  }
  return files;
}

const testFiles = (
  await Promise.all(
    packageNames.map((packageName) =>
      testFilesBelow(path.join(typescriptRoot, "packages", packageName, "test"))
    ),
  )
).flat().sort();

if (testFiles.length === 0) throw new Error("No TypeScript test files were discovered");

const result = spawnSync(process.execPath, ["--test", ...testFiles], {
  cwd: typescriptRoot,
  stdio: "inherit",
});

if (result.error !== undefined) throw result.error;
process.exitCode = result.status ?? 1;
