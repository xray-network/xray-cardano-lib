// Host-only Ed25519 comparison. Optional Noble package stays outside the project dependency graph.
import assert from 'node:assert/strict';
import { createHash, createPrivateKey, createPublicKey, sign, verify } from 'node:crypto';
import { readFileSync, writeFileSync, mkdirSync } from 'node:fs';
import { resolve, dirname } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';
import { cpus, release } from 'node:os';
import { spawn } from 'node:child_process';
import { once } from 'node:events';
import { createInterface } from 'node:readline';
import { sha512 } from '@noble/hashes/sha2.js';
import { secp256k1, schnorr } from '@noble/curves/secp256k1.js';
import { PrivateKey } from '@xray-network/xray-cardano-lib-crypto';

const here = dirname(fileURLToPath(import.meta.url));
const root = resolve(here, '../../../../..');
const nobleFile = resolve(process.env.XRAY_BENCH_NOBLE_ED25519);
const rustFile = resolve(process.env.XRAY_BENCH_RUST);
const noble = await import(pathToFileURL(nobleFile).href);
noble.hashes.sha512 = sha512;
const nobleManifest = JSON.parse(readFileSync(resolve(dirname(nobleFile), 'package.json')));
assert.equal(nobleManifest.name, '@noble/ed25519');
const digest = data => createHash('sha256').update(data).digest('hex');
const hashText = data => new Uint8Array(createHash('sha256').update(data).digest());
const hex = data => Buffer.from(data).toString('hex');
const fixtures = [], wire = { ed: [], secp: [], hash: [] };
for (let i = 0; i < 64; i++) {
  const seed = hashText(`xray-node-bench-test-seed:${i}`);
  const message = hashText(`xray-node-bench-message:${i}`);
  const xrayPrivate = PrivateKey.from_normal_bytes(seed), xrayPublic = xrayPrivate.to_public();
  const xraySignature = xrayPrivate.sign(message), signature = xraySignature.to_raw_bytes();
  const publicKey = xrayPublic.to_raw_bytes();
  const spki = Buffer.concat([Buffer.from('302a300506032b6570032100', 'hex'), publicKey]);
  const pkcs8 = Buffer.concat([Buffer.from('302e020100300506032b657004220420', 'hex'), seed]);
  const nodePublic = createPublicKey({ key: spki, type: 'spki', format: 'der' });
  const nodePrivate = createPrivateKey({ key: pkcs8, type: 'pkcs8', format: 'der' });
  assert.equal(hex(noble.getPublicKey(seed)), hex(publicKey));
  assert.equal(hex(noble.sign(message, seed)), hex(signature));
  assert.equal(hex(sign(null, message, nodePrivate)), hex(signature));
  const bad = message.slice(); bad[0] ^= 1;
  for (const [msg, valid] of [[message, true], [bad, false]]) {
    assert.equal(noble.verify(signature, msg, publicKey, { zip215: false }), valid);
    assert.equal(verify(null, msg, nodePublic, signature), valid);
    assert.equal(xrayPublic.verify(msg, xraySignature), valid);
  }
  fixtures.push({ seed, message, signature, publicKey, spki, nodePublic, nodePrivate, xrayPrivate, xrayPublic, xraySignature });
  wire.ed.push({ seed: hex(seed), message: hex(message), signature: hex(signature), public: hex(publicKey) });
}
// The existing Rust runner also initializes a secp fixture; it is not timed here.
const { seed, message } = fixtures[0];
wire.secp.push({ public: hex(secp256k1.getPublicKey(seed, true)), message: hex(message),
  signature: hex(secp256k1.sign(message, seed, { prehash: false, format: 'compact' })),
  schnorrPublic: hex(schnorr.getPublicKey(seed)), schnorrSignature: hex(schnorr.sign(message, seed, new Uint8Array(32))) });

const child = spawn(rustFile, [], { stdio: ['pipe', 'pipe', 'inherit'] });
child.on('error', error => { throw error; });
const lines = createInterface({ input: child.stdout })[Symbol.asyncIterator]();
async function ask(request) {
  const pending = lines.next(); child.stdin.write(JSON.stringify(request) + '\n');
  const response = await pending;
  if (response.done) throw new Error('Rust reference stopped');
  return JSON.parse(response.value);
}
assert.deepEqual(await ask(wire), { ready: true });
const cases = [
  { id: 'verify_rust_prepared', rust: 'ed25519_verify', expected: '01' },
  { id: 'verify_node_prepared', fn: f => verify(null, f.message, f.nodePublic, f.signature) },
  { id: 'verify_noble_raw', fn: f => noble.verify(f.signature, f.message, f.publicKey, { zip215: false }) },
  { id: 'verify_xray_wrappers', fn: f => f.xrayPublic.verify(f.message, f.xraySignature) },
  { id: 'verify_rust_fresh', rust: 'ed25519_verify_fresh', expected: '01' },
  { id: 'verify_node_fresh', fn: f => verify(null, f.message, createPublicKey({ key: f.spki, type: 'spki', format: 'der' }), f.signature) },
  { id: 'sign_rust_prepared', rust: 'ed25519_sign', expected: hex(fixtures[0].signature) },
  { id: 'sign_node_prepared', fn: f => sign(null, f.message, f.nodePrivate) },
  { id: 'sign_noble_seed', fn: f => noble.sign(f.message, f.seed) },
  { id: 'sign_xray_seed', fn: f => { const sig = f.xrayPrivate.sign(f.message); const raw = sig.to_raw_bytes(); sig.dispose(); return raw; } },
];
let sink;
async function sample(c, ms) {
  if (c.rust) {
    const result = await ask({ op: c.rust, ms, batch: 8 });
    assert.equal(result.output, c.expected); return result;
  }
  const start = performance.now(); let count = 0;
  do {
    for (let i = 0; i < 8; i++) sink = c.fn(fixtures[count++ % 64]);
  } while (performance.now() - start < ms);
  const elapsedMs = performance.now() - start;
  return { count, elapsedMs, opsPerSecond: count * 1000 / elapsedMs };
}
const samples = Object.fromEntries(cases.map(c => [c.id, []]));
try {
  for (const c of cases) await sample(c, 400);
  for (let round = 0; round < 7; round++) {
    let order = [...cases.slice(round), ...cases.slice(0, round)];
    if (round % 2) order.reverse();
    for (const c of order) samples[c.id].push(await sample(c, 300));
    process.stderr.write(`Ed25519 round ${round + 1}/7 complete\n`);
  }
} finally { child.stdin.end(); await once(child, 'exit'); }
const results = Object.fromEntries(Object.entries(samples).map(([name, windows]) => {
  const rates = windows.map(x => x.opsPerSecond).toSorted((a, b) => a - b);
  return [name, { opsPerSecond: rates[3], microsecondsPerOp: 1e6 / rates[3], minOpsPerSecond: rates[0], maxOpsPerSecond: rates.at(-1), samples: windows }];
}));
for (const [name, result] of Object.entries(results)) {
  const base = name.startsWith('sign_') ? results.sign_rust_prepared : name.endsWith('_fresh') ? results.verify_rust_fresh : results.verify_rust_prepared;
  result.percentOfRust = 100 * result.opsPerSecond / base.opsPerSecond;
}
const report = {
  metadata: { date: new Date().toISOString(), node: process.version, v8: process.versions.v8, openssl: process.versions.openssl,
    cpu: cpus()[0].model, logicalCpus: cpus().length, platform: process.platform, arch: process.arch, release: release(),
    nobleEd25519: nobleManifest.version, nobleHashes: '2.2.0', nobleCurvesUsedByXray: '2.2.0',
    nobleDistIntegrity: process.env.XRAY_BENCH_NOBLE_INTEGRITY ?? null,
    nobleSourceSha256: digest(readFileSync(nobleFile)), harnessSha256: digest(readFileSync(fileURLToPath(import.meta.url))),
    workspaceLockSha256: digest(readFileSync(resolve(root, 'libs/typescript/package-lock.json'))),
    fixtureSha256: digest(JSON.stringify(wire.ed)), rustSourceSha256: digest(readFileSync(resolve(here, 'rust-reference/src/main.rs'))),
    rustBinarySha256: digest(readFileSync(rustFile)), rustCompiler: process.env.XRAY_BENCH_RUST_VERSION ?? null,
    rustFlags: process.env.XRAY_BENCH_RUST_FLAGS ?? null,
    rustLockSha256: digest(readFileSync(resolve(here, 'rust-reference/Cargo.lock'))),
    rounds: 7, windowMs: 300, warmupMs: 400, fixtureCount: 64, messageBytes: 32,
    method: 'Synchronous APIs, one active execution thread, rotated/reversed ordering. IPC, fixture creation and DER byte construction excluded. No forced GC. Noble uses pure-JS @noble/hashes SHA-512 and zip215:false; Rust uses Dalek verify_strict. Prepared key objects differ internally; Noble verify always parses raw bytes. Signing uses a seed for Noble/XRAY versus existing key objects for Node/Dalek.' },
  results,
  limitations: ['Not all malformed-input semantics tested; no Cardano replacement decision implied.',
    'Noble async Web Crypto hashing and Node async/threadpool APIs not measured.',
    'Native Node crypto is a host-specific option; the XRAY package currently uses @noble/curves, not @noble/ed25519.',
    'No batching, parallel verification, Workers deployment, or full-node measurements.']
};
const output = resolve(process.argv[2] ?? resolve(here, 'results/ed25519-latest.json'));
mkdirSync(dirname(output), { recursive: true }); writeFileSync(output, JSON.stringify(report, null, 2) + '\n');
for (const [name, result] of Object.entries(results)) console.log(name, result.opsPerSecond.toFixed(1), result.percentOfRust.toFixed(2) + '%');
console.log('Saved', output);
for (const f of fixtures) { f.xrayPrivate.dispose(); f.xrayPublic.dispose(); f.xraySignature.dispose(); }
