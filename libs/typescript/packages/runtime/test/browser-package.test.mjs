import assert from "node:assert/strict";
import { readdir, readFile } from "node:fs/promises";
import { builtinModules } from "node:module";
import path from "node:path";
import { fileURLToPath } from "node:url";
import test from "node:test";

const typescriptRoot = fileURLToPath(new URL("../../", import.meta.url));
const packageRules = {
  chain: {
    dependencies: {
      "@xray-network/xray-cardano-lib-core": "0.1.0",
      "@xray-network/xray-cardano-lib-crypto": "0.1.0",
    },
    name: "@xray-network/xray-cardano-lib-chain",
  },
  cip: {
    dependencies: {
      "@xray-network/xray-cardano-lib-chain": "0.1.0",
      "@xray-network/xray-cardano-lib-core": "0.1.0",
      "@xray-network/xray-cardano-lib-crypto": "0.1.0",
    },
    name: "@xray-network/xray-cardano-lib-cip",
    subpaths: ["./cip25", "./cip36", "./cip8"],
  },
  core: {
    dependencies: {},
    name: "@xray-network/xray-cardano-lib-core",
  },
  crypto: {
    dependencies: {
      "@noble/ciphers": "2.2.0",
      "@noble/curves": "2.2.0",
      "@noble/hashes": "2.2.0",
      "@xray-network/xray-cardano-lib-core": "0.1.0",
    },
    name: "@xray-network/xray-cardano-lib-crypto",
  },
  runtime: {
    dependencies: {
      "@xray-network/xray-cardano-lib-chain": "0.1.0",
      "@xray-network/xray-cardano-lib-cip": "0.1.0",
      "@xray-network/xray-cardano-lib-core": "0.1.0",
      "@xray-network/xray-cardano-lib-crypto": "0.1.0",
      "@xray-network/xray-cardano-lib-plutus": "0.1.0",
    },
    name: "@xray-network/xray-cardano-lib",
  },
  plutus: {
    dependencies: {
      "@xray-network/xray-cardano-lib-chain": "0.1.0",
      "@xray-network/xray-cardano-lib-core": "0.1.0",
      "@xray-network/xray-cardano-lib-crypto": "0.1.0",
    },
    name: "@xray-network/xray-cardano-lib-plutus",
    subpaths: ["./data", "./uplc"],
  },
};
const nodeBuiltins = new Set(builtinModules.map((name) => name.replace(/^node:/u, "")));
const forbiddenGlobals = [
  [/\bBuffer\b/u, "Buffer"],
  [/\brequire\s*\(/u, "require()"],
  [/\bprocess(?:\.|\[)/u, "process"],
  [/\b__dirname\b/u, "__dirname"],
  [/\b__filename\b/u, "__filename"],
  [/\bmodule\.exports\b/u, "module.exports"],
  [/\bexports\./u, "CommonJS exports"],
  [/\bglobal\./u, "the Node global object"],
];

async function javascriptFilesBelow(directory) {
  const files = [];
  for (const entry of await readdir(directory, { withFileTypes: true })) {
    const entryPath = path.join(directory, entry.name);
    if (entry.isDirectory()) files.push(...await javascriptFilesBelow(entryPath));
    else if (entry.isFile() && entry.name.endsWith(".js")) files.push(entryPath);
  }
  return files;
}

function importedNodeBuiltins(source) {
  const imports = [];
  const patterns = [
    /\bfrom\s*["']([^"']+)["']/gu,
    /\bimport\s*["']([^"']+)["']/gu,
    /\bimport\s*\(\s*["']([^"']+)["']/gu,
    /\brequire\s*\(\s*["']([^"']+)["']/gu,
  ];
  for (const pattern of patterns) {
    for (const match of source.matchAll(pattern)) {
      const specifier = match[1];
      if (specifier === undefined) continue;
      const normalized = specifier.replace(/^node:/u, "");
      if (specifier.startsWith("node:") || nodeBuiltins.has(normalized)) imports.push(specifier);
    }
  }
  return imports;
}

test("all XRAY Cardano Lib packages expose the same universal ESM shape", async () => {
  const packageDirectories = Object.keys(packageRules).sort();
  assert.deepEqual(packageDirectories, Object.keys(packageRules).sort());

  for (const packageDirectory of packageDirectories) {
    const rule = packageRules[packageDirectory];
    assert.ok(rule !== undefined);
    const manifestUrl = new URL(`../../${packageDirectory}/package.json`, import.meta.url);
    const manifest = JSON.parse(await readFile(manifestUrl, "utf8"));
    assert.equal(manifest.name, rule.name);
    assert.deepEqual(manifest.dependencies ?? {}, rule.dependencies);
    assert.equal(manifest.license, "MIT");
    assert.equal(manifest.type, "module");
    assert.equal(manifest.main, "./dist/esm/index.js");
    assert.equal(manifest.exports["."].require, undefined);
    assert.equal(manifest.exports["."].browser, "./dist/esm/index.js");
    assert.equal(manifest.exports["."].import, "./dist/esm/index.js");
    assert.equal(manifest.exports["."].default, "./dist/esm/index.js");
    assert.equal(manifest.publishConfig.access, "public");
    assert.ok(manifest.files.includes("LICENSE"));
    for (const subpath of rule.subpaths ?? []) {
      assert.equal(manifest.exports[subpath].browser.startsWith("./dist/esm/"), true);
      assert.equal(manifest.exports[subpath].import, manifest.exports[subpath].browser);
      assert.equal(manifest.exports[subpath].default, manifest.exports[subpath].browser);
      assert.equal(typeof manifest.exports[subpath].types, "string");
    }
    if (packageDirectory === "plutus") {
      assert.equal(
        manifest.exports["./data"].browser,
        "./dist/esm/typed_data/index.js",
      );
      await assert.rejects(
        readdir(new URL("../../plutus/src/data/", import.meta.url)),
        { code: "ENOENT" },
      );
      await assert.rejects(
        readdir(new URL("../../plutus/dist/esm/data/", import.meta.url)),
        { code: "ENOENT" },
      );
    }
  }
});

test("emitted package ESM stays free of Node built-ins and globals", async () => {
  for (const packageDirectory of Object.keys(packageRules)) {
    const outputRoot = path.join(typescriptRoot, packageDirectory, "dist", "esm");
    const outputFiles = await javascriptFilesBelow(outputRoot);
    assert.ok(outputFiles.length > 0, `${packageDirectory} emitted no ESM`);

    for (const file of outputFiles) {
      const source = await readFile(file, "utf8");
      const relativeFile = path.relative(typescriptRoot, file);
      assert.deepEqual(
        importedNodeBuiltins(source),
        [],
        `${relativeFile} imports a Node built-in`,
      );
      for (const [pattern, name] of forbiddenGlobals) {
        assert.doesNotMatch(source, pattern, `${relativeFile} references ${name}`);
      }
    }
  }
});
