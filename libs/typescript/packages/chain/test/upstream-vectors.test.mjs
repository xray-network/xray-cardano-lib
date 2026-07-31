import assert from "node:assert/strict";
import { createHash } from "node:crypto";
import { readFile, readdir } from "node:fs/promises";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";

const workspaceRoot = fileURLToPath(new URL("../../../../../", import.meta.url));
const fixtureRoot = path.join(
  workspaceRoot,
  "updates",
  "providers",
  "cardano-multiplatform-lib",
  "0001-cardano-multiplatform-lib",
  "artifacts",
  "test-vectors",
);
const hexSha256 = /^[0-9a-f]{64}$/u;
const fixtureSourceRevision = "39681e0d435a71f7c47a2601507ab16e691abb9e";
const eraNames = new Map([
  [0, "byron-ebb"],
  [1, "byron-main"],
  [2, "shelley"],
  [3, "allegra"],
  [4, "mary"],
  [5, "alonzo"],
  [6, "babbage"],
  [7, "conway"],
]);

function sha256(bytes) {
  return createHash("sha256").update(bytes).digest("hex");
}

async function readJson(file) {
  return JSON.parse(await readFile(file, "utf8"));
}

async function filesBelow(directory) {
  const files = [];
  for (const entry of await readdir(directory, { withFileTypes: true })) {
    const resolved = path.join(directory, entry.name);
    if (entry.isDirectory()) files.push(...await filesBelow(resolved));
    else if (entry.isFile()) files.push(resolved);
  }
  return files;
}

function portableRelative(root, file) {
  return path.relative(root, file).split(path.sep).join("/");
}

function assertSafeRelative(relative, label) {
  assert.equal(path.isAbsolute(relative), false, `${label} must be relative`);
  assert.equal(relative.split("/").includes(".."), false, `${label} must not escape its root`);
}

function decodeStoredBlock(raw) {
  const text = raw.toString("utf8");
  if (/^[0-9a-fA-F\s]+$/u.test(text) && text.trim().length % 2 === 0) {
    return {
      bytes: Buffer.from(text.replace(/\s/gu, ""), "hex"),
      storageFormat: "ascii-hex",
    };
  }
  return { bytes: raw, storageFormat: "raw-cbor" };
}

test("upstream Cardano vectors retain exact provenance and checksums", async () => {
  const expectedCommit = fixtureSourceRevision;
  const inventory = await readJson(path.join(fixtureRoot, "manifest.json"));
  const provenance = await readJson(path.join(fixtureRoot, "PROVENANCE.json"));
  assert.equal(provenance.schemaVersion, 1, "unsupported fixture provenance schema");
  assert.equal(
    provenance.source?.repository,
    "https://github.com/dcSpark/cardano-multiplatform-lib.git",
    "fixture provenance repository mismatch",
  );
  assert.equal(provenance.source?.commit, expectedCommit, "fixture provenance commit mismatch");
  assert.equal(provenance.fixtureInventory, "manifest.json", "fixture inventory link mismatch");

  const blocks = inventory.fixtures?.goldenBlocks;
  const genesis = inventory.fixtures?.genesisJson;
  assert.equal(Array.isArray(blocks), true, "golden block fixture inventory is missing");
  assert.equal(Array.isArray(genesis), true, "genesis fixture inventory is missing");
  assert.equal(blocks.length, 86, "golden block fixture count must remain 86");
  assert.equal(genesis.length, 6, "genesis fixture count must remain 6");

  const entries = [...blocks, ...genesis];
  assert.equal(new Set(entries.map(({ path: sourcePath }) => sourcePath)).size, entries.length, "fixture source paths must be unique");
  assert.equal(new Set(entries.map(({ fixturePath }) => fixturePath)).size, entries.length, "tracked fixture paths must be unique");
  assert.deepEqual(
    blocks.map(({ path: sourcePath }) => sourcePath),
    blocks.map(({ path: sourcePath }) => sourcePath).toSorted((left, right) => left.localeCompare(right)),
    "golden block fixtures must remain deterministically ordered",
  );
  assert.deepEqual(
    genesis.map(({ path: sourcePath }) => sourcePath),
    genesis.map(({ path: sourcePath }) => sourcePath).toSorted((left, right) => left.localeCompare(right)),
    "genesis fixtures must remain deterministically ordered",
  );

  let fixtureBytes = 0;
  for (const fixture of blocks) {
    assertSafeRelative(fixture.path, `fixture source path ${fixture.path}`);
    assert.equal(
      fixture.fixturePath.endsWith(`/artifacts/test-vectors/${fixture.path}`),
      true,
      `${fixture.path} has an inconsistent captured path`,
    );
    const raw = await readFile(path.join(fixtureRoot, fixture.path));
    const decoded = decodeStoredBlock(raw);
    fixtureBytes += raw.length;
    assert.equal(decoded.bytes[0], 0x82, `${fixture.path} is not a two-item explicit-network envelope`);
    assert.equal(eraNames.has(decoded.bytes[1]), true, `${fixture.path} has an unknown era tag`);
    const expected = {
      path: fixture.path,
      fixturePath: fixture.fixturePath,
      source: fixture.path.startsWith("blocks/mainnet/")
        ? "dolos@ea7960a1c2e56c523fec7c4bab75f390ee443514"
        : "pallas@a7b5a86",
      eraTag: decoded.bytes[1],
      era: eraNames.get(decoded.bytes[1]),
      storageFormat: decoded.storageFormat,
      storedBytes: raw.length,
      decodedCborBytes: decoded.bytes.length,
      sha256: sha256(raw),
      expected: fixture.path.endsWith("/conway8.block")
        ? "reject-invalid-hash-size"
        : "byte-exact-round-trip",
    };
    assert.deepEqual(fixture, expected, `${fixture.path} metadata does not match its tracked bytes`);
  }

  for (const fixture of genesis) {
    assertSafeRelative(fixture.path, `fixture source path ${fixture.path}`);
    assert.equal(
      fixture.fixturePath.endsWith(`/artifacts/test-vectors/${fixture.path}`),
      true,
      `${fixture.path} has an inconsistent captured path`,
    );
    const raw = await readFile(path.join(fixtureRoot, fixture.path));
    JSON.parse(raw.toString("utf8"));
    fixtureBytes += raw.length;
    assert.deepEqual(fixture, {
      path: fixture.path,
      fixturePath: fixture.fixturePath,
      bytes: raw.length,
      sha256: sha256(raw),
      expected: "parse-success",
    }, `${fixture.path} metadata does not match its tracked bytes`);
  }

  const supplemental = provenance.supplementalSha256;
  assert.equal(typeof supplemental, "object", "supplemental fixture provenance hashes are missing");
  assert.equal(Array.isArray(supplemental), false, "supplemental fixture hashes must be a map");
  const supplementalPaths = Object.keys(supplemental).sort((left, right) => left.localeCompare(right));
  assert.deepEqual(supplementalPaths, ["PROVENANCE.md"]);
  for (const relative of supplementalPaths) {
    assertSafeRelative(relative, `supplemental fixture path ${relative}`);
    assert.match(supplemental[relative], hexSha256, `invalid supplemental fixture SHA-256 for ${relative}`);
    assert.equal(sha256(await readFile(path.join(fixtureRoot, relative))), supplemental[relative], `supplemental fixture checksum mismatch for ${relative}`);
  }

  const controlFiles = new Set([
    ".gitattributes",
    "LICENSE-APACHE-2.0.txt",
    "manifest.json",
    "PROVENANCE.json",
    "README.md",
  ]);
  const expectedImported = [...entries.map(({ path: sourcePath }) => sourcePath), ...supplementalPaths]
    .sort((left, right) => left.localeCompare(right));
  const actualImported = (await filesBelow(fixtureRoot))
    .map((file) => portableRelative(fixtureRoot, file))
    .filter((relative) => !controlFiles.has(relative))
    .sort((left, right) => left.localeCompare(right));
  assert.deepEqual(actualImported, expectedImported, "fixture inventory must cover the exact imported file tree");

  assert.equal(blocks.filter(({ expected }) => expected === "byte-exact-round-trip").length, 85);
  assert.equal(blocks.filter(({ expected }) => expected !== "byte-exact-round-trip").length, 1);
  assert.deepEqual(provenance.licenses, [
    {
      paths: ["blocks/mainnet/**"],
      sourceRepository: "https://github.com/txpipe/dolos.git",
      revision: "ea7960a1c2e56c523fec7c4bab75f390ee443514",
      license: "Apache-2.0",
      licenseFile: "LICENSE-APACHE-2.0.txt",
    },
    {
      paths: ["blocks/pallas/**"],
      sourceRepository: "https://github.com/txpipe/pallas.git",
      revision: "a7b5a86e3922ea46723e7959118293232db7bf3a",
      license: "Apache-2.0",
      licenseFile: "LICENSE-APACHE-2.0.txt",
    },
  ], "fixture source/license mappings do not match the pinned provenance");
  assert.match(provenance.licenseSha256, hexSha256, "fixture license SHA-256 is invalid");
  assert.equal(
    sha256(await readFile(path.join(fixtureRoot, "LICENSE-APACHE-2.0.txt"))),
    provenance.licenseSha256,
    "fixture Apache-2.0 license checksum mismatch",
  );
  assert.deepEqual(
    (await readFile(path.join(workspaceRoot, ".gitattributes"), "utf8"))
      .split("\n")
      .filter(Boolean)
      .sort(),
    [
      "/updates/providers/*/*/artifacts/** -text",
    ],
    "root Git attributes must protect imported bytes from line-ending conversion",
  );
  assert.equal(
    await readFile(path.join(fixtureRoot, ".gitattributes"), "utf8"),
    "**/*.block -text\n**/*.cbor -text\n**/*.json -text\n",
    "fixture-local Git attributes drifted",
  );

  assert.ok(fixtureBytes > 0, "fixture corpus must not be empty");
});
