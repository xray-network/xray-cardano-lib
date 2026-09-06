# Contributing to XRAY Cardano Lib

XRAY Cardano Lib is a language-first polyglot repository. Source, provider evidence, and implementation
history have separate ownership:

- `libs/<language>/` owns implementation source, packages, tests, manifests, and validation.
- `.agents/spectre/implementations/<language>/` owns active numbered instructions and results.
- `SPECTRE.md` owns aggregate active lifecycle state; `.agents/spectre/archive/` owns terminal history.
- `.agents/spectre/providers/` owns shared provider contracts and captured evidence.
- `docs/` owns the Rspress site and architecture decisions.

The maintained implementations are TypeScript under `libs/typescript/` and C++ under `libs/cpp/`.
Each owns an independent implementation sequence represented in the aggregate lifecycle ledger.

## Before making a change

1. Read the root `README.md`; on an explicit SPECTRE invocation, follow `.agents/skills/spectre/SKILL.md`.
2. Read `docs/README.md` and relevant active ADRs.
3. Read `.agents/spectre/templates/TEMPLATE_IMPL.md` and
   `.agents/spectre/templates/TEMPLATE_STATUS.md` for
   implementation-ledger work.
4. Read the owning library or package README, manifest, source, and tests.
5. For provider work, read `.agents/spectre/templates/TEMPLATE_PROVIDER.md` and the selected provider
   contract.

There is no root build command. Each language owns its workspace and completion gate.

## Implementation ownership

- Keep each implementation below `libs/<language>/`.
- Keep language-owned packages below `libs/<language>/packages/`.
- Keep shared protocol semantics consistent with declared inputs while allowing language-native
  representations and errors where no shared contract requires otherwise.
- Keep a public nominal type in one owning package. Facades re-export it without copying,
  wrapping, subclassing, or regenerating it.
- Store tests with the package that owns the behavior.
- Never generate implementation source, exports, tests, or package metadata from provider
  artifacts or another library's result.

## Implementation records

- `SPECTRE.md` is the sole active lifecycle and decision-proof authority for every library target
  and follows `.agents/spectre/templates/TEMPLATE_STATUS.md`.
- Each bounded active change uses a matching pair directly under `.agents/spectre/implementations/<language>/`:
  `NNNN-IMPL-INSTR.md` and `NNNN-IMPL-RESULT.md`.
- The instruction declares `DIRECT`, `DERIVED`, `HYBRID`, or `LOCAL` evidence mode.
- Preparation creates the complete instruction and `PLANNED` row without changing source.
- Implementation consumes only declared inputs, creates the result, records validation, and moves
  the row to `REVIEW`.
- Only a human moves `REVIEW` to `ACCEPTED` or `REJECTED`.
- Terminal rows and their instruction/result pair are immutable.
- Results include an exported, language-neutral change contract for optional downstream use.

## Cross-library inputs

A library may consume an accepted result from another library, use provider evidence directly, or
combine both. Declare those inputs only in the local implementation instruction and record actual
consumption in its paired result. Keep each aggregate status section limited to implementation
records owned by that target.

## Provider evidence

- Provider contracts live below `.agents/spectre/providers/<provider>/`.
- Any library may consume shared provider evidence. Provider contracts and snapshots record
  provenance; consuming instructions and results record library-local use.
- A provider snapshot contains immutable `SNAPSHOT.md` and nonempty `artifacts/`.
- Provider snapshots contain no implementation instruction, result, status, or changelog.
- Preparation never changes implementation source.
- Direct and hybrid implementations consume captured evidence without fetching, refreshing,
  executing, or substituting upstream material.
- Captured source is evidence, not repository tooling or generated source.

## Documentation and ADRs

- Add Rspress pages below `docs/src/` and update `docs/rspress.config.ts` navigation.
- Put shared decisions in `docs/src/adr/repository/` and language decisions in
  `docs/src/adr/<language>/`.
- Keep the root README focused on the repository and implementation model.
- Keep package details in their owning README.
- Keep active implementation records under `.agents/spectre/implementations/` and terminal records
  under immutable `.agents/spectre/archive/` batches.
- Treat existing `docs/src/impl/` pages as historical XRAY Updates mirrors, not active SPECTRE records.
- Update relative links when content moves.

## TypeScript implementation

The TypeScript workspace lives under `libs/typescript/`.

- Use `Uint8Array`, `bigint`, Web Crypto-compatible interfaces, and Web Platform APIs.
- Published code must not import `node:*`, call `require`, reference `Buffer`, or contain native
  or WebAssembly artifacts.
- Preserve lossless CBOR metadata and test preserved and canonical encoding separately.
- Treat cryptographic boundaries, malformed encodings, randomness, secrets, and browser
  compatibility as security-sensitive.

Run targeted checks while working. Before finishing a TypeScript change, run:

```sh
npm --prefix libs/typescript run check
```
