// Host-only measurement tooling; never imported by a published package.
import assert from 'node:assert/strict';
import { createHash } from 'node:crypto';
import { readFileSync, writeFileSync, mkdirSync } from 'node:fs';
import { cpus, platform, arch, release } from 'node:os';
import { resolve, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
import { execFileSync, spawn } from 'node:child_process';
import { createInterface } from 'node:readline';
import { once } from 'node:events';
import { Worker, isMainThread, parentPort } from 'node:worker_threads';
import * as core from '@xray-network/xray-cardano-lib-core';
import * as crypto from '@xray-network/xray-cardano-lib-crypto';
import * as chain from '@xray-network/xray-cardano-lib-chain';
import { MultiEraBlock } from '@xray-network/xray-cardano-lib-chain/multi-era';
import * as plutus from '@xray-network/xray-cardano-lib-plutus';
import { makeScriptContext } from '../../../packages/plutus/dist/esm/ledger/context.js';
import { secp256k1, schnorr } from '@noble/curves/secp256k1.js';
import { bls12_381 } from '@noble/curves/bls12-381.js';

const directory = dirname(fileURLToPath(import.meta.url));
const root = resolve(directory, '../../../../..');
const hex = value => Buffer.from(value).toString('hex');
const digest = value => createHash('sha256').update(value).digest('hex');
const hashText = value => new Uint8Array(createHash('sha256').update(value).digest());
const u = value => ({ kind: 'unsigned', value: BigInt(value), encoding: { width: 0 } });
const b = value => ({ kind: 'bytes', value, encoding: { kind: 'definite', width: 0 } });
const a = values => ({ kind: 'array', values, encoding: { kind: 'definite', width: 0 } });
const m = entries => ({ kind: 'map', entries, encoding: { kind: 'definite', width: 0 } });
const constant = (type, value) => ({ kind: 'constant', constant: { type, value } });
const apply = (fn, argument) => ({ kind: 'apply', function: fn, argument });
const maxBudget = { cpu: 0x7fff_ffff_ffff_ffffn, memory: 0x7fff_ffff_ffff_ffffn };
const json = value => JSON.stringify(value, (_, x) => typeof x === 'bigint' ? x.toString() : x, 2);
const median = values => values.toSorted((x, y) => x - y)[Math.floor(values.length / 2)];
let sink;

// Independent raw jobs: 32-byte key, 32-byte message, 64-byte signature.
if (!isMainThread) {
  parentPort.on('message', raw => {
    const bytes = new Uint8Array(raw);
    let accepted = 0;
    for (let offset = 0; offset < bytes.length; offset += 128) {
      const key = crypto.PublicKey.from_bytes(bytes.subarray(offset, offset + 32));
      const sig = crypto.Ed25519Signature.from_raw_bytes(bytes.subarray(offset + 64, offset + 128));
      accepted += Number(key.verify(bytes.subarray(offset + 32, offset + 64), sig));
      key.dispose(); sig.dispose();
    }
    parentPort.postMessage(accepted);
  });
  parentPort.postMessage('ready');
} else {
  await main();
}

function window(fn, ms, batch = 8) {
  let count = 0;
  const start = performance.now();
  do {
    for (let i = 0; i < batch; i++) sink = fn(count++);
  } while (performance.now() - start < ms);
  const elapsedMs = performance.now() - start;
  return { count, elapsedMs, opsPerSecond: count * 1000 / elapsedMs };
}
function stats(samples) {
  const rates = samples.map(x => x.opsPerSecond);
  const opsPerSecond = median(rates);
  return { opsPerSecond, microsecondsPerOp: 1e6 / opsPerSecond,
    minOpsPerSecond: Math.min(...rates), maxOpsPerSecond: Math.max(...rates), samples };
}

async function main() {
  const smoke = process.argv.includes('--smoke');
  const outputArg = process.argv.indexOf('--output');
  const output = outputArg < 0 ? resolve(directory, 'results/latest.json') : resolve(process.argv[outputArg + 1]);
  const rounds = smoke ? 1 : 7, windowMs = smoke ? 10 : 300, warmupMs = smoke ? 10 : 400;
  const cases = [], fixtures = { ed: [], secp: [], hash: [] };
  const provenance = [], cryptoObjects = [], findings = [];
  const add = (id, fn, details = {}) => cases.push({ id, fn, ...details });

  for (let i = 0; i < 64; i++) {
    const seed = hashText(`xray-node-bench-test-seed:${i}`);
    const message = hashText(`xray-node-bench-message:${i}`);
    const key = crypto.PrivateKey.from_normal_bytes(seed);
    const publicKey = key.to_public(), signature = key.sign(message);
    assert(publicKey.verify(message, signature));
    const bad = message.slice(); bad[0] ^= 1;
    assert.equal(publicKey.verify(bad, signature), false);
    fixtures.ed.push({ seed: hex(seed), message: hex(message), public: hex(publicKey.to_raw_bytes()), signature: hex(signature.to_raw_bytes()) });
    cryptoObjects.push({ key, publicKey, signature, message, rawKey: publicKey.to_raw_bytes(), rawSignature: signature.to_raw_bytes() });
    if (i < 16) {
      const publicBytes = secp256k1.getPublicKey(seed, true);
      const ecdsa = secp256k1.sign(message, seed, { prehash: false, format: 'compact' });
      const schnorrPublic = schnorr.getPublicKey(seed), schnorrSignature = schnorr.sign(message, seed, new Uint8Array(32));
      assert(crypto.verifySecp256k1Ecdsa(publicBytes, ecdsa, message));
      assert(crypto.verifySecp256k1Schnorr(schnorrPublic, schnorrSignature, message));
      assert.equal(crypto.verifySecp256k1Ecdsa(publicBytes, ecdsa, bad), false);
      assert.equal(crypto.verifySecp256k1Schnorr(schnorrPublic, schnorrSignature, bad), false);
      fixtures.secp.push({ public: hex(publicBytes), signature: hex(ecdsa), message: hex(message), schnorrPublic: hex(schnorrPublic), schnorrSignature: hex(schnorrSignature) });
    }
  }
  const ed = i => cryptoObjects[i % 64];
  add('ed25519_verify', i => { const e = ed(i); return e.publicKey.verify(e.message, e.signature); }, { rust: true, unit: 'checks', output: '01' });
  add('ed25519_verify_fresh', i => {
    const e = ed(i), key = crypto.PublicKey.from_bytes(e.rawKey), sig = crypto.Ed25519Signature.from_raw_bytes(e.rawSignature);
    const valid = key.verify(e.message, sig); key.dispose(); sig.dispose(); return valid;
  }, { rust: true, unit: 'checks', output: '01' });
  add('ed25519_sign', i => { const e = ed(i), sig = e.key.sign(e.message); const raw = sig.to_raw_bytes(); sig.dispose(); return raw; },
    { rust: true, unit: 'signatures', output: fixtures.ed[0].signature });
  const secp = fixtures.secp.map(x => Object.fromEntries(Object.entries(x).map(([k, v]) => [k, new Uint8Array(Buffer.from(v, 'hex'))])));
  add('secp256k1_ecdsa_verify', i => { const c = secp[i % 16]; return crypto.verifySecp256k1Ecdsa(c.public, c.signature, c.message); }, { rust: true, unit: 'checks', output: '01' });
  add('secp256k1_schnorr_verify', i => { const c = secp[i % 16]; return crypto.verifySecp256k1Schnorr(c.schnorrPublic, c.schnorrSignature, c.message); }, { rust: true, unit: 'checks', output: '01' });
  for (const size of [32, 1024, 65536]) {
    const data = Uint8Array.from({ length: size }, (_, i) => (i * 17 + 11) & 255);
    fixtures.hash.push(hex(data));
    for (const name of ['blake2b256', 'blake2b224', 'sha2_256', 'sha3_256']) {
      add(`${name}_${size}`, () => crypto[name](data), { rust: true, bytesPerOp: size, unit: 'hashes', output: hex(crypto[name](data)) });
    }
  }
  const g1 = bls12_381.G1.Point.BASE.toBytes(true), g2 = bls12_381.G2.Point.BASE.toBytes(true);
  const scalar = BigInt('0x' + fixtures.ed[0].seed) % bls12_381.fields.Fr.ORDER;
  const groupProduct = crypto.bls12_381_scalar_mul('g1', scalar, g1);
  assert.deepEqual(groupProduct, bls12_381.G1.Point.BASE.multiply(scalar).toBytes(true));
  const ml = crypto.bls12_381_miller_loop(g1, g2);
  const mlLeft = crypto.bls12_381_mul_ml_result(ml, crypto.bls12_381_miller_loop(bls12_381.G1.Point.BASE.multiply(2n).toBytes(true), g2));
  const mlRight = crypto.bls12_381_miller_loop(bls12_381.G1.Point.BASE.multiply(3n).toBytes(true), g2);
  assert.notEqual(hex(mlLeft), hex(mlRight));
  assert(crypto.bls12_381_final_verify(mlLeft, mlRight));
  add('bls_g1_scalar_mul', () => crypto.bls12_381_scalar_mul('g1', scalar, g1), { unit: 'operations', note: 'Compressed point parse, scalar multiply and compressed output; fixed nontrivial scalar.' });
  add('bls_miller_loop', () => crypto.bls12_381_miller_loop(g1, g2), { unit: 'operations', note: 'Compressed point parse and Miller loop; no final exponentiation.' });
  add('bls_final_verify', () => crypto.bls12_381_final_verify(mlLeft, mlRight), { unit: 'operations', note: 'Distinct Miller outputs whose final exponentiations agree: e(P,Q)*e(2P,Q)=e(3P,Q).' });

  const fixtureRoot = resolve(root, '.agents/spectre/providers/cardano-multiplatform-lib/0001/artifacts/test-vectors');
  const manifest = JSON.parse(readFileSync(resolve(fixtureRoot, 'manifest.json')));
  for (const era of ['alonzo', 'babbage', 'conway']) {
    const candidates = manifest.fixtures.goldenBlocks.filter(x => x.era === era && x.expected === 'byte-exact-round-trip').toSorted((x, y) => x.decodedCborBytes - y.decodedCborBytes);
    const fixture = candidates.at(-1); // Largest accepted captured block in each selected era.
    const raw = readFileSync(resolve(fixtureRoot, fixture.path));
    assert.equal(digest(raw), fixture.sha256);
    const data = new Uint8Array(fixture.storageFormat === 'ascii-hex' ? Buffer.from(raw.toString('utf8').replace(/\s/g, ''), 'hex') : raw);
    assert.equal(data.length, fixture.decodedCborBytes);
    const node = core.decodeCbor(data), block = MultiEraBlock.from_explicit_network_cbor_bytes(data);
    assert.equal(hex(core.encodeCbor(node)), hex(data));
    assert.equal(hex(block.to_explicit_network_cbor_bytes()), hex(data));
    provenance.push({ ...fixture, decodedSha256: digest(data) });
    add(`cbor_decode_${era}`, () => core.decodeCbor(data), { bytesPerOp: data.length, unit: 'blocks', fixture: fixture.path });
    add(`cbor_encode_${era}`, () => core.encodeCbor(node), { bytesPerOp: data.length, unit: 'blocks', fixture: fixture.path, note: 'Preserved encoding of an already decoded tree.' });
    add(`block_decode_${era}`, () => MultiEraBlock.from_explicit_network_cbor_bytes(data), { bytesPerOp: data.length, unit: 'blocks', fixture: fixture.path, note: 'Wire shape validation only; no signatures, scripts or ledger state transition.' });
  }

  let term = constant({ kind: 'integer' }, 0n);
  for (let i = 0; i < 256; i++) term = apply(apply({ kind: 'builtin', tag: 0 }, term), constant({ kind: 'integer' }, 1n));
  const arithmetic = { version: [1n, 0n, 0n], term };
  const ar = plutus.evaluateProgram(arithmetic, [], maxBudget);
  assert.equal(ar.value.constant.value, 256n);
  const flat = plutus.encodeFlatProgram(arithmetic);
  assert.deepEqual(plutus.decodeFlatProgram(flat), arithmetic);
  add('flat_decode_256_adds', () => plutus.decodeFlatProgram(flat), { unit: 'programs', bytesPerOp: flat.length });
  add('flat_encode_256_adds', () => plutus.encodeFlatProgram(arithmetic), { unit: 'programs', bytesPerOp: flat.length });
  add('cek_256_adds', () => plutus.evaluateProgram(arithmetic, [], maxBudget), { unit: 'scripts', budget: ar.budget, note: 'Default machine costs, semantics A; AST already decoded; includes costing and result discharge.' });
  for (const size of [256, 4096]) {
    let list = constant({ kind: 'list', item: { kind: 'integer' } }, Array.from({ length: size }, (_, i) => BigInt(i)));
    for (let i = 0; i < size; i++) list = apply({ kind: 'force', term: { kind: 'builtin', tag: 34 } }, list);
    const program = { version: [1n, 0n, 0n], term: list };
    const result = plutus.evaluateProgram(program, [], maxBudget);
    assert.deepEqual(result.value.constant.value, []);
    add(`cek_${size}_tails`, () => plutus.evaluateProgram(program, [], maxBudget), { unit: 'scripts', budget: result.budget, note: 'Take every tail of an integer list; default machine costs and semantics A.' });
  }

  for (const size of [1, 16, 64]) {
    const f = phaseFixture(size);
    const current = { tag: 0, index: 0n, data: u(7) };
    const context = () => makeScriptContext(current, [current], f.body, f.witnesses, f.utxos, [0n, 0n, 1000n], 5, 0);
    assert(core.encodeCbor(context()).length > 0);
    add(`context_v1_${size}_inputs`, context, { unit: 'contexts', note: 'Private context owner, decoded inputs; V1/protocol 5; one script input, remaining key inputs; no database lookup.' });
    const evaluate = () => plutus.evaluatePhaseTwoRaw(f.txBytes, f.utxos.map(x => [x.inputBytes, x.outputBytes]), f.costBytes, [10000000n, 10000000n], [0n, 0n, 1000n], 5, true);
    const result = evaluate();
    assert.equal(result.length, 1);
    assert.deepEqual(result[0][1].cost, { cpu: 209100n, memory: 1100n });
    add(`phase_two_v1_${size}_inputs`, evaluate, { unit: 'transactions', budget: result[0][1].cost, note: 'Synthetic one-redeemer unit-returning script with synthetic cost model; includes raw decode, resolution, context and evaluation; not full ledger validation.' });
    if (size === 16) {
      const tx = chain.Transaction.from_cbor_bytes(f.txBytes);
      const legacyResolved = f.utxos.map(x => chain.TransactionUnspentOutput.new(chain.TransactionInput.from_cbor_bytes(x.inputBytes), chain.TransactionOutput.from_cbor_bytes(x.outputBytes)));
      try { chain.discover_required_witnesses(tx, legacyResolved); }
      catch (error) { findings.push({ operation: 'discover_required_witnesses', fixture: '16 resolved legacy array outputs with datum hashes', error: error.message }); }
      const resolvedInputs = f.utxos.map(x => chain.TransactionUnspentOutput.new(
        chain.TransactionInput.from_cbor_bytes(x.inputBytes),
        chain.TransactionOutput.new(chain.Address.from_raw_bytes(x.output.values[0].value), chain.Value.from_coin(1000000n),
          chain.DatumOption.new(0n, crypto.DatumHash.from_raw_bytes(x.output.values[2].value)))));
      const discover = () => chain.discover_required_witnesses(tx, resolvedInputs);
      assert.equal(discover().redeemers.size, 1);
      add('witness_discovery_16_inputs', discover, { unit: 'transactions', note: 'Modern map-form resolved outputs; requirements discovery, no signature or full phase-one validation.' });
      const fee = chain.LinearFee.new(44n, 155381n);
      assert.equal(chain.min_no_script_fee(tx, fee), BigInt(f.txBytes.length) * 44n + 155381n);
      add('linear_fee_16_inputs', () => chain.min_no_script_fee(tx, fee), { unit: 'transactions', note: 'Includes serialization; excludes execution and reference-script fees.' });
      const outputModel = chain.TransactionOutput.from_cbor_bytes(f.utxos[0].outputBytes);
      assert(chain.min_ada_required(outputModel, 4310n) > 0n);
      add('min_utxo_ada', () => chain.min_ada_required(outputModel, 4310n), { unit: 'outputs' });
    }
  }

  let child, lines;
  const ask = async request => {
    const next = lines.next(); child.stdin.write(JSON.stringify(request) + '\n');
    const response = await next;
    if (response.done) throw new Error('Rust reference exited before returning a result');
    return JSON.parse(response.value);
  };
  const rustBinary = process.env.XRAY_BENCH_RUST;
  if (rustBinary) {
    child = spawn(resolve(rustBinary), [], { stdio: ['pipe', 'pipe', 'inherit'] });
    child.on('error', error => { throw error; });
    lines = createInterface({ input: child.stdout })[Symbol.asyncIterator]();
    assert.deepEqual(await ask(fixtures), { ready: true });
  }
  const metadata = {
    date: new Date().toISOString(), sourceCommit: execFileSync('git', ['rev-parse', 'HEAD'], { cwd: root, encoding: 'utf8' }).trim(),
    libraryChanges: execFileSync('git', ['diff', '--name-only', '--', 'libs/typescript/packages', 'libs/typescript/package-lock.json'], { cwd: root, encoding: 'utf8' }).trim(),
    platform: platform(), release: release(), arch: arch(), cpu: cpus()[0].model, logicalCpus: cpus().length,
    versions: process.versions, lockSha256: digest(readFileSync(resolve(root, 'libs/typescript/package-lock.json'))),
    harnessSha256: digest(readFileSync(fileURLToPath(import.meta.url))), fixtureSha256: digest(JSON.stringify(fixtures)),
    rustSourceSha256: digest(readFileSync(resolve(directory, 'rust-reference/src/main.rs'))),
    rustLockSha256: rustBinary ? digest(readFileSync(resolve(directory, 'rust-reference/Cargo.lock'))) : null,
    rustBinarySha256: rustBinary ? digest(readFileSync(resolve(rustBinary))) : null,
    rustCompiler: process.env.XRAY_BENCH_RUST_VERSION ?? null,
    rustFlags: process.env.XRAY_BENCH_RUST_FLAGS ?? null,
    rounds, windowMs, warmupMs, smoke, blockFixtures: provenance,
    method: 'One process doing timed work at a time. Warm each path, seven alternating JS/Rust windows. Report median throughput. No forced GC. Not per-operation latency percentiles. Rust release build; inputs prepared outside timing.'
  };
  const results = [];
  try {
    for (const c of cases) {
      window(c.fn, warmupMs);
      const matched = Boolean(rustBinary && c.rust);
      if (matched) assert.equal((await ask({ op: c.id, ms: warmupMs, batch: 8 })).output, c.output);
      const js = [], rust = [];
      for (let round = 0; round < rounds; round++) {
        const runRust = async () => {
          const sample = await ask({ op: c.id, ms: windowMs, batch: 8 });
          assert.equal(sample.output, c.output); rust.push(sample);
        };
        if (matched && round % 2) await runRust();
        js.push(window(c.fn, windowMs));
        if (matched && !(round % 2)) await runRust();
      }
      const { fn, rust: needsRust, output: expected, ...details } = c;
      const result = { ...details, typescript: stats(js), rust: matched ? stats(rust) : null };
      if (matched) result.typescriptPercentOfRust = 100 * result.typescript.opsPerSecond / result.rust.opsPerSecond;
      results.push(result);
      process.stderr.write(`${c.id}: ${result.typescript.opsPerSecond.toFixed(1)}/s${matched ? `; Rust=100%, TS=${result.typescriptPercentOfRust.toFixed(2)}%` : ''}\n`);
    }
  } finally { if (child) { child.stdin.end(); await once(child, 'exit'); } }

  const workers = [];
  for (const count of smoke ? [1, 2] : [1, 2, 4, 8]) {
    const pool = Array.from({ length: count }, () => new Worker(new URL(import.meta.url)));
    await Promise.all(pool.map(worker => once(worker, 'message')));
    const payload = new Uint8Array(64 * 128);
    for (let i = 0; i < 64; i++) {
      payload.set(cryptoObjects[i].rawKey, i * 128);
      payload.set(cryptoObjects[i].message, i * 128 + 32);
      payload.set(cryptoObjects[i].rawSignature, i * 128 + 64);
    }
    async function dispatch(jobs) {
      let next = 0, accepted = 0;
      const start = performance.now();
      await Promise.all(pool.map(async worker => {
        while (next < jobs) {
          next++;
          const message = once(worker, 'message'), copy = payload.slice();
          worker.postMessage(copy.buffer, [copy.buffer]);
          const [verified] = await message;
          accepted += verified;
        }
      }));
      const elapsedMs = performance.now() - start;
      assert.equal(accepted, jobs * 64);
      return { count: accepted, elapsedMs, opsPerSecond: accepted * 1000 / elapsedMs };
    }
    try {
      await dispatch(count * (smoke ? 1 : 8));
      const samples = [];
      for (let i = 0; i < (smoke ? 1 : 5); i++) samples.push(await dispatch(smoke ? 2 : 32));
      workers.push({ threads: count, ...stats(samples) });
      process.stderr.write(`workers=${count}: ${workers.at(-1).opsPerSecond.toFixed(1)} checks/s\n`);
    } finally { await Promise.all(pool.map(worker => worker.terminate())); }
  }
  const report = { metadata, results, workers, findings,
    limitations: ['Microbenchmarks, not a validating Cardano node or Cloudflare deployment.',
      'Crypto equivalence checked on canonical valid fixtures and modified-message rejection only; not all malformed input semantics.',
      'Prepared Ed25519 keys have different internal representations. Rust stores a parsed curve point; XRAY wrappers store bytes.',
      'Unimplemented P2P, consensus, VRF/KES, full ledger, epochs, database and block production were not measured.',
      'No Rust CBOR/UPLC/BLS comparator, memory comparison, p99 latency or energy measurement.',
      'Worker pools are synthetic host tooling with transferable 64-check jobs; warm pools, startup excluded, copying, messaging and wrappers included. Rust remains a single-thread crypto reference.'] };
  mkdirSync(dirname(output), { recursive: true }); writeFileSync(output, json(report) + '\n');
  process.stderr.write(`Saved ${output}\n`);
  for (const e of cryptoObjects) { e.key.dispose(); e.publicKey.dispose(); e.signature.dispose(); }
}

function phaseFixture(size) {
  const unit = constant({ kind: 'unit' }, null);
  const script = plutus.encodeProgramEnvelope({ version: [1n, 0n, 0n], term: { kind: 'lambda', body: { kind: 'lambda', body: { kind: 'lambda', body: unit } } } });
  const scriptHash = crypto.blake2b224(Uint8Array.of(1, ...script));
  const datum = u(42), datumHash = crypto.blake2b256(core.encodeCbor(datum));
  const utxos = Array.from({ length: size }, (_, i) => {
    const input = a([b(hashText(`xray-node-bench-input:${i}`)), u(0)]);
    const address = Uint8Array.of(i === 0 ? 0x70 : 0x60, ...(i === 0 ? scriptHash : new Uint8Array(28).fill(i)));
    const output = a([b(address), u(1000000), b(datumHash)]);
    return { input, output, inputBytes: core.encodeCbor(input), outputBytes: core.encodeCbor(output) };
  });
  // Redeemer index uses ledger input ordering, not the generated array order.
  utxos.sort((x, y) => Buffer.compare(x.input.values[0].value, y.input.values[0].value));
  // Keep the script input first in canonical order for a stable spending pointer.
  for (let i = 0; i < utxos.length; i++) {
    utxos[i].output.values[0] = b(Uint8Array.of(i === 0 ? 0x70 : 0x60, ...(i === 0 ? scriptHash : new Uint8Array(28).fill(i))));
    utxos[i].outputBytes = core.encodeCbor(utxos[i].output);
  }
  const body = m([[u(0), a(utxos.map(x => x.input))], [u(1), a([])], [u(2), u(0)]]);
  const redeemer = a([u(0), u(0), u(7), a([u(0), u(0)])]);
  const witnesses = m([[u(3), a([b(script)])], [u(4), a([datum])], [u(5), a([redeemer])]]);
  const txBytes = core.encodeCbor(a([body, witnesses, { kind: 'boolean', value: true }, { kind: 'null' }]));
  const parameters = Array.from({ length: 332 }, () => 1n);
  for (const [index, value] of [[17,16000],[18,100],[19,23000],[20,100],[21,23000],[22,100],[23,23000],[24,100],[25,23000],[26,100],[27,23000],[28,100],[29,100],[30,100],[31,23000],[32,100]]) parameters[index] = BigInt(value);
  const costBytes = core.encodeCbor(m([[u(0), a(parameters.map(u))]]));
  return { body, witnesses, utxos, txBytes, costBytes };
}
