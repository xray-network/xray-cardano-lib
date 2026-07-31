import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import test from "node:test";

const workspaceManifest = JSON.parse(
  await readFile(new URL("../../../package.json", import.meta.url), "utf8"),
);
const lock = JSON.parse(
  await readFile(new URL("../../../package-lock.json", import.meta.url), "utf8"),
);
const expectedExternal = {
  "node_modules/@noble/ciphers": {
    license: "MIT",
    version: "2.2.0",
  },
  "node_modules/@noble/curves": {
    license: "MIT",
    version: "2.2.0",
  },
  "node_modules/@noble/hashes": {
    license: "MIT",
    version: "2.2.0",
  },
  "node_modules/typescript": {
    license: "Apache-2.0",
    version: "5.9.3",
  },
};

test("external dependencies remain reviewed, pinned, and lockfile-integrity protected", () => {
  assert.deepEqual(workspaceManifest.devDependencies, { typescript: "5.9.3" });
  assert.deepEqual(lock.packages[""].devDependencies, workspaceManifest.devDependencies);

  const externalLocations = Object.entries(lock.packages)
    .filter(([location, value]) => location.startsWith("node_modules/") && value.link !== true)
    .map(([location]) => location)
    .sort();
  assert.deepEqual(externalLocations, Object.keys(expectedExternal).sort());

  for (const [location, expected] of Object.entries(expectedExternal)) {
    const dependency = lock.packages[location];
    assert.ok(dependency !== undefined, `${location} is absent from package-lock.json`);
    assert.equal(dependency.version, expected.version, `${location} version`);
    assert.equal(dependency.license, expected.license, `${location} license`);
    assert.match(
      dependency.integrity ?? "",
      /^sha512-[A-Za-z0-9+/=]+$/u,
      `${location} lock integrity`,
    );
  }
});
