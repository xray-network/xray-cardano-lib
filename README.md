# XRAY Cardano Lib

XRAY Cardano Lib is a repository for Cardano protocol and application libraries. It combines modular
language ownership with captured provider evidence and auditable implementation histories.

TypeScript is the only maintained implementation. The C++ workspace is retained as a concept for
experimentation and design exploration; it is not maintained, supported, or kept in feature
parity with TypeScript.

The project emphasizes lossless data handling, explicit package boundaries, browser-compatible
runtime behavior, reproducible dependencies, and evidence-backed compatibility.

Repository-wide guidance for contributors and coding agents is documented in
[`AGENTS.md`](./AGENTS.md). This repository adopts
[`SPECTRE 1.0.0`](./.agents/spectre/SPECTRE-PROTOCOL.md) for evidence-backed implementation tracking.

## Current libraries

| Library | Description | Source | Implementation history |
| --- | --- | --- | --- |
| TypeScript | The only maintained implementation, covering encoding, cryptography, ledger eras, transactions, CIPs, Plutus Data, and UPLC | [`libs/typescript/`](./libs/typescript/) | [SPECTRE archive](./.agents/spectre/archive/20260906T201907Z/ARCHIVE.md) |
| C++ | Unmaintained C++23 concept retained for experimentation and design exploration; not a supported feature-parity implementation | [`libs/cpp/`](./libs/cpp/) | [SPECTRE archive](./.agents/spectre/archive/20260906T201907Z/ARCHIVE.md) |

## Project model

- `libs/` contains language-owned source, manifests, tests, and implementation documentation.
- `.agents/spectre/implementations/<language>/` contains active numbered instructions and results.
- `SPECTRE.md` is the aggregate active lifecycle ledger; terminal history is preserved below
  `.agents/spectre/archive/`.
- `.agents/spectre/providers/` contains shared provider contracts and immutable captured evidence.
- `docs/` contains the Rspress documentation package, architecture decisions, and implementation
  instruction/result mirrors.

Every active implementation change uses a pair directly below the SPECTRE implementation directory:

```text
.agents/spectre/implementations/<language>/NNNN-IMPL-INSTR.md
.agents/spectre/implementations/<language>/NNNN-IMPL-RESULT.md
```

The instruction defines the bounded objective and inputs. The result records actual changes,
validation, deviations, and a portable semantic change contract. Each target owns an independent
sequence represented in the aggregate status ledger.

## Evidence choices

An implementation instruction explicitly selects one evidence mode:

- `DIRECT`: use provider evidence.
- `DERIVED`: use accepted results from another library.
- `HYBRID`: use both.
- `LOCAL`: use neither.

This allows each library record to identify its evidence independently. TypeScript is the active
implementation target. Historical C++ records and source remain available as concept material,
without an ongoing maintenance or parity commitment.

Provider evidence is language-neutral and may be consumed by any implementation. Provider
contracts and snapshots record capture provenance; instructions and results record library-local
consumption. Captured artifacts remain evidence rather than generated source or runtime
dependencies.

See the [implementation overview](./.agents/spectre/README.md),
[implementation workflow](./.agents/spectre/templates/TEMPLATE_IMPL.md),
[status schema](./.agents/spectre/templates/TEMPLATE_STATUS.md),
[provider workflow](./.agents/spectre/templates/TEMPLATE_PROVIDER.md), and the
[active ledger](./SPECTRE.md).

## Repository layout

```text
libs/
  typescript/
  cpp/

.agents/
  skills/spectre/SKILL.md
  spectre/
    SPECTRE-PROTOCOL.md
    README.md
    templates/
      TEMPLATE_IMPL.md
      TEMPLATE_PROVIDER.md
      TEMPLATE_STATUS.md
    implementations/             # active records, empty until work is planned
    archive/                      # terminal history
    providers/
      <provider>/
        PROVIDER.md
        <snapshot>/
          SNAPSHOT.md
          artifacts/

docs/
  src/
    adr/
    impl/                         # historical XRAY documentation snapshot
      typescript/
        0001-IMPL-INSTR.md
        0001-IMPL-RESULT.md
```
