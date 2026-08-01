# Contributing to XRAY Cardano Lib

XRAY Cardano Lib is a language-first polyglot repository. Source, provider evidence, and implementation
history have separate ownership:

- `libs/<language>/` owns implementation source, packages, tests, manifests, and validation.
- `.xray/updates/implementations/<language>/` owns that library's numbered instructions and results.
- `.xray/updates/XRAY-UPDATES-STATUS.md` owns aggregate lifecycle state for every target.
- `.xray/updates/providers/` owns shared provider contracts and captured evidence.
- `docs/` owns the Mintlify site and architecture decisions.

The maintained implementations are TypeScript under `libs/typescript/` and C++ under `libs/cpp/`.
Each owns an independent implementation sequence represented in the aggregate lifecycle ledger.

## Before making a change

1. Read the root `README.md` and `.xray/updates/XRAY-UPDATES.md`.
2. Read `docs/README.md` and relevant active ADRs.
3. Read `.xray/updates/templates/TEMPLATE_IMPL.md` and
   `.xray/updates/templates/TEMPLATE_STATUS.md` for
   implementation-ledger work.
4. Read the owning library or package README, manifest, source, and tests.
5. For provider work, read `.xray/updates/templates/TEMPLATE_PROVIDER.md` and the selected provider
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

- `.xray/updates/XRAY-UPDATES-STATUS.md` is the sole lifecycle and decision-proof authority for
  every library target and follows `.xray/updates/templates/TEMPLATE_STATUS.md`.
- Each bounded change uses a matching pair directly under `.xray/updates/implementations/<language>/`:
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

- Provider contracts live below `.xray/updates/providers/<provider>/`.
- Any library may consume shared provider evidence. Provider contracts and snapshots record
  provenance; consuming instructions and results record library-local use.
- A provider snapshot contains immutable `SNAPSHOT.md` and nonempty `artifacts/`.
- Provider snapshots contain no implementation instruction, result, status, or changelog.
- Preparation never changes implementation source.
- Direct and hybrid implementations consume captured evidence without fetching, refreshing,
  executing, or substituting upstream material.
- Captured source is evidence, not repository tooling or generated source.

## Documentation and ADRs

- Add Mintlify pages below `docs/` and update `docs/docs.json` navigation.
- Put shared decisions in `docs/adr/repository/` and language decisions in
  `docs/adr/<language>/`.
- Keep the root README focused on the repository and implementation model.
- Keep package details in their owning README.
- Keep canonical implementation records under `.xray/updates/implementations/`.
- Mirror the aggregate status at `docs/impl/XRAY-UPDATES-STATUS.md` and numbered instructions and
  results from `.xray/updates/implementations/<target>/` below `docs/impl/<target>/` for Mintlify.
- Never mirror providers, artifacts, or record templates.
- Never edit a documentation mirror independently. Update it in the same change as its canonical
  source. Preserve canonical text except for deterministic link changes required to reach
  non-mirrored canonical records.
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
