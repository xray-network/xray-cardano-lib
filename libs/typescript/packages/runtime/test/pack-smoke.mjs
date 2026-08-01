import { spawnSync } from "node:child_process";
import { mkdir, mkdtemp, readFile, rm, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";

const typescriptRoot = fileURLToPath(new URL("../../../", import.meta.url));
const packages = [
  ["core", "@xray-network/xray-cardano-lib-core"],
  ["crypto", "@xray-network/xray-cardano-lib-crypto"],
  ["chain", "@xray-network/xray-cardano-lib-chain"],
  ["cip", "@xray-network/xray-cardano-lib-cip"],
  ["plutus", "@xray-network/xray-cardano-lib-plutus"],
  ["runtime", "@xray-network/xray-cardano-lib"],
];
const temporaryRoot = await mkdtemp(path.join(tmpdir(), "xray-cardano-lib-pack-smoke-"));
let packedFileCount = 0;
let packedByteCount = 0;

function run(command, args, workingDirectory) {
  const result = spawnSync(command, args, {
    cwd: workingDirectory,
    encoding: "utf8",
    env: {
      ...process.env,
      npm_config_audit: "false",
      npm_config_cache: path.join(temporaryRoot, "npm-cache"),
      npm_config_fund: "false",
    },
  });
  if (result.status !== 0) {
    throw new Error(
      `${command} ${args.join(" ")} failed\n${result.stdout ?? ""}${result.stderr ?? ""}`,
    );
  }
  return result.stdout;
}

try {
  const tarballs = [];
  for (const [packageDirectory, expectedName] of packages) {
    const output = run(
      "npm",
      [
        "pack",
        "--json",
        "--pack-destination",
        temporaryRoot,
        path.join(typescriptRoot, "packages", packageDirectory),
      ],
      typescriptRoot,
    );
    const [packResult] = JSON.parse(output);
    if (packResult === undefined) throw new Error(`npm pack returned no result for ${packageDirectory}`);
    packedFileCount += packResult.entryCount;
    packedByteCount += packResult.unpackedSize;

    const forbiddenFile = packResult.files.find(({ path: filePath }) =>
      /(?:^|\/)(?:Cargo\.(?:lock|toml)|.*\.(?:node|rs|wasm))$/u.test(filePath),
    );
    if (forbiddenFile !== undefined) {
      throw new Error(`${packageDirectory} tarball contains forbidden ${forbiddenFile.path}`);
    }
    if (!packResult.files.some(({ path: filePath }) => filePath === "LICENSE")) {
      throw new Error(`${packageDirectory} tarball does not contain LICENSE`);
    }
    if (!packResult.files.some(({ path: filePath }) => filePath === "README.md")) {
      throw new Error(`${packageDirectory} tarball does not contain README.md`);
    }
    const unintendedFile = packResult.files.find(({ path: filePath }) =>
      filePath !== "package.json" &&
      filePath !== "LICENSE" &&
      filePath !== "README.md" &&
      filePath !== "NOTICE" &&
      !/^dist\/esm\/.+\.(?:js|d\.ts|map|json)$/u.test(filePath)
    );
    if (unintendedFile !== undefined) {
      throw new Error(`${packageDirectory} tarball contains unintended ${unintendedFile.path}`);
    }

    const manifest = JSON.parse(
      await readFile(path.join(typescriptRoot, "packages", packageDirectory, "package.json"), "utf8"),
    );
    if (manifest.name !== expectedName) {
      throw new Error(`${packageDirectory} has package name ${manifest.name}; expected ${expectedName}`);
    }
    tarballs.push(path.join(temporaryRoot, packResult.filename));
  }

  for (const dependency of ["@noble/ciphers", "@noble/curves", "@noble/hashes"]) {
    const output = run(
      "npm",
      [
        "pack",
        "--json",
        "--pack-destination",
        temporaryRoot,
        path.join(typescriptRoot, "node_modules", ...dependency.split("/")),
      ],
      typescriptRoot,
    );
    const [packResult] = JSON.parse(output);
    if (packResult === undefined) throw new Error(`npm pack returned no result for ${dependency}`);
    const binary = packResult.files.find(({ path: filePath }) =>
      /\.(?:a|dll|dylib|node|o|so|wasm)$/u.test(filePath),
    );
    if (binary !== undefined) throw new Error(`${dependency} tarball contains ${binary.path}`);
    tarballs.push(path.join(temporaryRoot, packResult.filename));
  }

  const consumerRoot = path.join(temporaryRoot, "consumer");
  await mkdir(consumerRoot);
  await writeFile(
    path.join(consumerRoot, "package.json"),
    '{\n  "name": "xray-cardano-lib-pack-smoke-consumer",\n  "private": true,\n  "type": "module"\n}\n',
  );
  run(
    "npm",
    ["install", "--offline", "--ignore-scripts", "--no-audit", "--no-fund", ...tarballs],
    consumerRoot,
  );

  await writeFile(
    path.join(consumerRoot, "esm-smoke.mjs"),
    [
      'import * as cardano from "@xray-network/xray-cardano-lib";',
      'import * as chain from "@xray-network/xray-cardano-lib-chain";',
      'import * as multiEra from "@xray-network/xray-cardano-lib-chain/multi-era";',
      'import { BabbageTransactionBody } from "@xray-network/xray-cardano-lib-chain/babbage";',
      'import * as core from "@xray-network/xray-cardano-lib-core";',
      'import * as cip from "@xray-network/xray-cardano-lib-cip";',
      'import { CIP25ChunkableString } from "@xray-network/xray-cardano-lib-cip/cip25";',
      'import { CIP36KeyDeregistration } from "@xray-network/xray-cardano-lib-cip/cip36";',
      'import { COSESign1Builder, HeaderMap, Headers, ProtectedHeaderMap } from "@xray-network/xray-cardano-lib-cip/cip8";',
      'import * as plutus from "@xray-network/xray-cardano-lib-plutus";',
      'import { Data } from "@xray-network/xray-cardano-lib-plutus/data";',
      'import * as uplc from "@xray-network/xray-cardano-lib-plutus/uplc";',
      'import { PrivateKey } from "@xray-network/xray-cardano-lib-crypto";',
      "if (cardano.CardanoError !== core.CardanoError ||",
      "    cardano.MultiEraBlock !== multiEra.MultiEraBlock ||",
      "    cardano.BabbageTransactionBody !== BabbageTransactionBody) {",
      '  throw new Error("packages did not preserve nominal class identity");',
      "}",
      'const historicalBodyHex = "bf00800180020009a2581c61616161616161616161616161616161616161616161616161616161a14001581c61616161616161616161616161616161616161616161616161616161a14002ff";',
      'const historicalBody = BabbageTransactionBody.from_cbor_bytes(Buffer.from(historicalBodyHex, "hex"));',
      'if (Buffer.from(historicalBody.to_cbor_bytes()).toString("hex") !== historicalBodyHex) {',
      '  throw new Error("packed multi-era lossless CBOR failed");',
      "}",
      "const key = PrivateKey.from_normal_bytes(new Uint8Array(32).fill(1));",
      'if (!key.to_public().verify(new Uint8Array([1]), key.sign(new Uint8Array([1])))) {',
      '  throw new Error("packed crypto dependency graph failed signing verification");',
      "}",
      'if (CIP25ChunkableString.from_string("x".repeat(65)).as_chunked()?.len() !== 2) {',
      '  throw new Error("packed CIP-25 dependency graph failed chunking");',
      "}",
      "if (CIP36KeyDeregistration.new(key.to_public(), 1n).hash_to_sign(false).length !== 32) {",
      '  throw new Error("packed CIP-36 dependency graph failed signing payload creation");',
      "}",
      "const cip8Headers = Headers.new(ProtectedHeaderMap.new_empty(), HeaderMap.new());",
      "const cip8Builder = COSESign1Builder.new(cip8Headers, new Uint8Array([1, 2, 3]), false);",
      "const cip8SigningData = cip8Builder.make_data_to_sign().to_cbor_bytes();",
      "const cip8Signature = key.sign(cip8SigningData);",
      "const cip8Signed = cip8Builder.build(cip8Signature.to_raw_bytes());",
      "if (!key.to_public().verify(cip8Signed.signed_data().to_cbor_bytes(), cip8Signature)) {",
      '  throw new Error("packed CIP-8 Sign1 flow failed");',
      "}",
      'if (chain.PlutusData.from_json(\'{"int":42}\').as_integer()?.to_str() !== "42") {',
      '  throw new Error("packed chain Plutus data parser failed");',
      "}",
      'const datumSchema = Data.Object({ owner: Data.Bytes(), amount: Data.Integer() });',
      'const datumValue = { owner: "abcd", amount: 42n };',
      "if (Data.from(Data.to(datumValue, datumSchema), datumSchema).amount !== 42n) {",
      '  throw new Error("packed Plutus schema codec failed");',
      "}",
      'const uplcProgram = uplc.parseUplcText("(program 1.0.0 (con unit ()))");',
      'const uplcResult = uplc.evaluateProgram(uplc.decodeFlatProgram(uplc.encodeFlatProgram(uplcProgram)), [], { cpu: 1000000n, memory: 1000000n });',
      "if (!uplcResult.isUnit || plutus.parseUplcText !== uplc.parseUplcText) {",
      '  throw new Error("packed UPLC parser, Flat codec, or evaluator failed");',
      "}",
      "if (cip.cip25.CIP25Metadata === undefined || cip.cip8.COSESign1Builder !== COSESign1Builder || plutus.Data !== Data) {",
      '  throw new Error("packed CIP or Plutus root entry point failed");',
      "}",
      "",
    ].join("\n"),
  );
  run(process.execPath, ["esm-smoke.mjs"], consumerRoot);

  await writeFile(
    path.join(consumerRoot, "types-smoke.ts"),
    [
      'import * as cardano from "@xray-network/xray-cardano-lib";',
      'import * as chain from "@xray-network/xray-cardano-lib-chain";',
      'import * as allegra from "@xray-network/xray-cardano-lib-chain/allegra";',
      'import * as alonzo from "@xray-network/xray-cardano-lib-chain/alonzo";',
      'import * as babbage from "@xray-network/xray-cardano-lib-chain/babbage";',
      'import * as byron from "@xray-network/xray-cardano-lib-chain/byron";',
      'import * as conway from "@xray-network/xray-cardano-lib-chain/conway";',
      'import * as mary from "@xray-network/xray-cardano-lib-chain/mary";',
      'import * as multiEra from "@xray-network/xray-cardano-lib-chain/multi-era";',
      'import * as shelley from "@xray-network/xray-cardano-lib-chain/shelley";',
      'import type { AllegraBlockJSON } from "@xray-network/xray-cardano-lib-chain/allegra";',
      'import type { AlonzoBlockJSON } from "@xray-network/xray-cardano-lib-chain/alonzo";',
      'import type { BabbageBlockJSON } from "@xray-network/xray-cardano-lib-chain/babbage";',
      'import type { ByronBlockJSON } from "@xray-network/xray-cardano-lib-chain/byron";',
      'import type { BlockJSON as ConwayBlockJSON } from "@xray-network/xray-cardano-lib-chain/conway";',
      'import type { MaryBlockJSON } from "@xray-network/xray-cardano-lib-chain/mary";',
      'import type { ShelleyBlockJSON } from "@xray-network/xray-cardano-lib-chain/shelley";',
      'import * as cip from "@xray-network/xray-cardano-lib-cip";',
      'import * as cip25 from "@xray-network/xray-cardano-lib-cip/cip25";',
      'import * as cip36 from "@xray-network/xray-cardano-lib-cip/cip36";',
      'import * as cip8 from "@xray-network/xray-cardano-lib-cip/cip8";',
      'import * as plutus from "@xray-network/xray-cardano-lib-plutus";',
      'import { Data } from "@xray-network/xray-cardano-lib-plutus/data";',
      'import * as uplc from "@xray-network/xray-cardano-lib-plutus/uplc";',
      'import * as core from "@xray-network/xray-cardano-lib-core";',
      'import * as crypto from "@xray-network/xray-cardano-lib-crypto";',
      'import type { PlutusDataJSON, TransactionMetadatumJSON } from "@xray-network/xray-cardano-lib";',
      'import type { MultiEraBlockJSON } from "@xray-network/xray-cardano-lib-chain/multi-era";',
      "const datum: PlutusDataJSON = { constructor: 0, fields: [{ int: 42 }] };",
      'const metadata: TransactionMetadatumJSON = { string: "xray-cardano-lib" };',
      "const block = { Conway: {} as never } satisfies MultiEraBlockJSON;",
      "const datumSchema = Data.Object({ owner: Data.Bytes(), amount: Data.Integer() });",
      "type DatumValue = Data.Static<typeof datumSchema>;",
      "type EraBlockJSON = ByronBlockJSON | ShelleyBlockJSON | AllegraBlockJSON | MaryBlockJSON | AlonzoBlockJSON | BabbageBlockJSON | ConwayBlockJSON;",
      'const datumValue: DatumValue = { owner: "abcd", amount: 42n };',
      "const eraBlock = null as unknown as EraBlockJSON;",
      "Data.to(datumValue, datumSchema);",
      "const cip8Headers = cip8.Headers.new(cip8.ProtectedHeaderMap.new_empty(), cip8.HeaderMap.new());",
      "const cip8Builder = cip8.COSESign1Builder.new(cip8Headers, new Uint8Array(), false);",
      "void cip8Builder.make_data_to_sign();",
      'const uplcProgram = uplc.parseUplcText("(program 1.0.0 (con unit ()))");',
      "const uplcResult: uplc.MachineResult = uplc.evaluateProgram(uplcProgram, [], { cpu: 1000000n, memory: 1000000n });",
      "void [cardano, chain, allegra, alonzo, babbage, byron, conway, mary, multiEra, shelley, cip, cip25, cip36, cip8, plutus, uplc, core, crypto, datum, metadata, block, eraBlock, uplcResult];",
      "",
    ].join("\n"),
  );
  const compiler = path.join(typescriptRoot, "node_modules", "typescript", "bin", "tsc");
  run(process.execPath, [compiler, "--noEmit", "--strict", "--skipLibCheck", "--target", "ES2022", "--module", "NodeNext", "--moduleResolution", "NodeNext", "types-smoke.ts"], consumerRoot);
  run(process.execPath, [compiler, "--noEmit", "--strict", "--skipLibCheck", "--target", "ES2022", "--module", "ESNext", "--moduleResolution", "Bundler", "types-smoke.ts"], consumerRoot);

  for (const [, packageName] of packages) {
    const [, unscopedName] = packageName.split("/");
    const manifest = JSON.parse(
      await readFile(
        path.join(consumerRoot, "node_modules", "@xray-network", unscopedName, "package.json"),
        "utf8",
      ),
    );
    if (
      manifest.exports?.["."]?.require !== undefined ||
      manifest.exports?.["."]?.import !== "./dist/esm/index.js" ||
      manifest.exports?.["."]?.browser !== "./dist/esm/index.js"
    ) {
      throw new Error(`${packageName} is not a universal ESM package`);
    }
  }

  process.stdout.write(`Packed XRAY Cardano Lib package smoke tests passed: ${packedFileCount} intended files, ${packedByteCount} unpacked bytes, ESM + NodeNext + bundler consumers.\n`);
} finally {
  if (process.env.KEEP_PACK_SMOKE !== "1") {
    await rm(temporaryRoot, { force: true, recursive: true });
  } else {
    process.stdout.write(`Kept pack smoke directory: ${temporaryRoot}\n`);
  }
}
