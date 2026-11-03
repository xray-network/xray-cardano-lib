# Node component measurements

Host-only benchmarks for the node architecture in [NODE-PLAN.md](../NODE-PLAN.md).
These tools do not change the published library's pure TypeScript dependency policy. The Rust
program is a benchmark reference, not a new maintained Cardano implementation or runtime dependency.

## Reproduce

From the repository root, with the TypeScript workspace dependencies installed:

```sh
npm --prefix libs/typescript run check
RUSTFLAGS='-C target-cpu=native' cargo build --locked --release --manifest-path libs/typescript/tools/node-investigation/node-bench/rust-reference/Cargo.toml
XRAY_BENCH_RUST=libs/typescript/tools/node-investigation/node-bench/rust-reference/target/release/xray-node-bench-reference XRAY_BENCH_RUST_FLAGS='-C target-cpu=native' node libs/typescript/tools/node-investigation/node-bench/run.mjs --output /tmp/xray-node-components.json
```

Set `XRAY_BENCH_RUST_VERSION` to the compiler version used to build that binary. If using
`CARGO_TARGET_DIR`, point `XRAY_BENCH_RUST` at its `release/xray-node-bench-reference` file.
Omit `XRAY_BENCH_RUST` for absolute TypeScript measurements only. Use `--smoke` to check fixture
construction, expected outputs and worker job accounting; smoke timings are not performance results.
Run benchmarks after the build/check completes, with other CPU-intensive work stopped.

The recorded run used Rust 1.98.1, target `aarch64-apple-darwin`, LLVM 22.1.8, release optimization,
thin LTO, one codegen unit and `-C target-cpu=native`. The toolchain and Cargo downloads/build
outputs were isolated under `/private/tmp/xray-node-perf`; no installed user toolchain was changed.
Runtime dependencies remain untouched. `Cargo.lock` pins the reference's complete dependency graph.

## Scope and method

- Seven windows of at least 300 ms after 400 ms warmup per implementation and case. JS and Rust
  alternate execution order; only one performs timed work at a time. Every eight operations the
  harness checks elapsed time. Raw counts and elapsed time are saved. Median window throughput is
  the headline metric; reciprocal throughput is an amortized time, not p50 or p99 request latency.
- Rust uses `black_box`; JS retains the last result. Inputs and keys are constructed before timing,
  except where a case explicitly includes parsing. Rust IPC/startup and JSON output are outside
  timing. Both sides include dispatch, result construction and loop overhead; these are API-level
  measurements, not isolated assembly kernels. No forced GC, cache flushing or CPU affinity.
- 64 deterministic Ed25519 seeds/messages, 16 secp256k1 fixtures, and fixed 32 B/1 KiB/64 KiB hash
  inputs. Seeds are test-only SHA-256 of `xray-node-bench-test-seed:` plus decimal index; messages
  use `xray-node-bench-message:`. Signatures agree across implementations, canonical valid signatures
  pass and altered messages fail. Hash output equality is checked in each Rust window.
- Rust references: `ed25519-dalek 2.2.0` / `curve25519-dalek 4.1.3`, `k256 0.13.4`, `blake2 0.10.6`,
  `sha2 0.10.9` and `sha3 0.10.8`. These are specific pinned Rust libraries, not Amaru. A different
  backend or build can yield different results. A valid-input corpus does not prove equivalent
  malformed-input, subgroup or consensus acceptance rules.
- Prepared Ed25519 compares public API objects: Dalek retains a parsed point, while XRAY's wrappers
  retain bytes. The fresh case imports the public key and signature on each operation. Signing
  includes public API costs: Dalek retains expanded signing material; XRAY owns its seed and expands
  it in signing. Neither signing case uses extended/BIP32 keys. secp256k1 cases include raw key and
  signature parsing in both implementations, with ECDSA prehashing disabled and raw BIP-340 messages.
  The separately validated UPLC wrappers may do extra parsing and are not timed by these kernel cases.
- BLS tests include G1 scalar multiplication with a nontrivial scalar, a Miller loop, and final
  verification of distinct Miller outputs satisfying `e(P,Q)*e(2P,Q)=e(3P,Q)`. No Rust BLS ratio.
- CBOR uses the largest accepted captured block of each of Alonzo, Babbage and Conway. These three
  blocks are a small diagnostic set, not a representative chain sample. Original file checksums and
  decoded-byte checksums are recorded; exact preserved round trips are asserted. Captured files
  are read as data only. `MultiEraBlock` performs wire/model validation, not consensus/ledger validation.
- UPLC workloads are synthetic: 256 integer additions and complete tail traversals of 256/4096-item
  lists. Default machine costs, semantics A, already decoded ASTs, cost accounting and result discharge
  included. Flat codec timings are separate. Runtime budget consumption is saved, not treated as time.
- V1 context and phase-two measurements use protocol 5, one unit-returning spending script, 1/16/64
  resolved inputs and a synthetic cost model. Context-only timing begins with decoded CBOR; raw
  phase-two includes decode, resolution, context construction, evaluation and rewritten redeemer output.
  Its phase-one option checks redeemer presence, not full phase-one ledger validity. No real contract
  or current-mainnet transaction throughput claim is supported by these fixtures.
- Worker tests use warm pools of 1/2/4/8 JS workers, five rounds of 2,048 checks, 64 checks per job,
  with at most one outstanding job per worker. Timing includes payload copies, transfer, fresh wrappers,
  coordination and responses. Pool creation and warmup are excluded. The same total work is performed
  at every width. This is not a node worker pool, nor an equal-thread Rust comparison.

The JSON contains environment, source and harness identities, fixture checksums, every measurement
window, percentages where a Rust reference actually ran, worker results and correctness findings.
`typescriptPercentOfRust = 100 * medianJsOps / medianRustOps`; higher is better. Do not average these
percentages or multiply the one-thread Rust reference into an unmeasured multithreaded result.

## Boundaries and observed correctness issue

P2P, consensus, VRF/KES, stateful ledger validation, rewards/epochs, the proposed database, producer
and end-to-end relay are not implemented in this workspace and have no measurements here. Cloudflare
Workers, network I/O, memory, startup, p99 latency and energy are also unmeasured. Node timings must
not be presented as deployed Worker performance.

The harness records a failure of `discover_required_witnesses` on valid resolved legacy array
outputs carrying datum hashes. In `chain/src/builder/transaction.ts`, `outputParts` passes the CBOR
encoding of the datum-hash bytes to `DatumOption.new`, which expects the 32 raw bytes. The encoded
value is 34 bytes, causing `DatumOption[1] requires 32-byte CBOR bytes`. The timed discovery case
uses semantically equivalent modern map-form outputs. Raw phase-two accepts the legacy fixtures.
This observation is recorded for follow-up; the benchmark does not change library behavior.

Reference API documentation: [Dalek strict verification](https://docs.rs/ed25519-dalek/2.2.0/ed25519_dalek/struct.VerifyingKey.html#method.verify_strict),
[k256 raw Schnorr verification](https://docs.rs/k256/0.13.4/k256/schnorr/struct.VerifyingKey.html#method.verify_raw),
[Blake2 variable output](https://docs.rs/blake2/0.10.6/blake2/).

## Direct Ed25519 comparison

`ed25519.mjs` compares Node/OpenSSL, standalone `@noble/ed25519 3.2.0`, the existing Rust
reference and XRAY on the same 64 seeds/messages. Install the extra package only in a temporary
benchmark directory, outside the workspace dependency graph:

```sh
npm install --prefix /tmp/xray-ed25519-compare --ignore-scripts --no-audit --no-fund --save-exact @noble/ed25519@3.2.0
XRAY_BENCH_NOBLE_ED25519=/tmp/xray-ed25519-compare/node_modules/@noble/ed25519/index.js XRAY_BENCH_RUST=libs/typescript/tools/node-investigation/node-bench/rust-reference/target/release/xray-node-bench-reference node libs/typescript/tools/node-investigation/node-bench/ed25519.mjs /tmp/xray-ed25519-results.json
```

Build the Rust reference as described above. Set `XRAY_BENCH_RUST_VERSION` and
`XRAY_BENCH_RUST_FLAGS` to record compiler metadata. `XRAY_BENCH_NOBLE_INTEGRITY` can record
the package integrity string from the temporary npm lockfile; the harness always records the
loaded Noble JavaScript file's SHA-256. The recorded 3.2.0 release integrity is
`sha512-criDgRlnUA09hchYrTy/JUWPIEap5rZxQe6wDWzRx51oWWpDRcUpuNzlgPxDJaOK6AsW9c0wKcj3rKRv6t+bPQ==`.

All APIs are synchronous. Noble is configured with `@noble/hashes 2.2.0` SHA-512 and
`zip215: false`, per its [documented API](https://github.com/paulmillr/noble-ed25519#usage).
This is different from the project's existing `@noble/curves 2.2.0` implementation. The harness
checks public keys, signatures and changed-message rejection before measurement, then runs seven
300 ms windows after 400 ms warmup, rotating and reversing case order. The prepared verification
table compares public API representations; Noble's verifier always parses raw bytes. The fresh
verification rows additionally import Node/Dalek public keys each operation; Node DER bytes are
prepared outside timing. Signing uses existing Node/Dalek key objects versus raw seeds in
Noble/XRAY. Signing and verification have separate Rust=100% baselines, and the fresh-import rows
use fresh Rust verification as their baseline. Async APIs and full malformed-input equivalence
remain unmeasured.
