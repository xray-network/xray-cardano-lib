# XRAY Cardano Lib

XRAY Cardano Lib is a repository for Cardano protocol and application libraries. It combines modular
language ownership with captured provider evidence and auditable implementation histories.

TypeScript is the only maintained implementation. The C++ workspace is retained as a concept for
experimentation and design exploration; it is not maintained, supported, or kept in feature
parity with TypeScript.

The project emphasizes lossless data handling, explicit package boundaries, browser-compatible
runtime behavior, reproducible dependencies, and evidence-backed compatibility.

Repository-wide guidance for contributors and coding agents is documented in
[`AGENTS.md`](./AGENTS.md).

## Current libraries

| Library | Description | Source | Implementation history |
| --- | --- | --- | --- |
| TypeScript | The only maintained implementation, covering encoding, cryptography, ledger eras, transactions, CIPs, Plutus Data, and UPLC | [`libs/typescript/`](./libs/typescript/) | [`updates/implementations/typescript/`](./updates/implementations/typescript/) |
| C++ | Unmaintained C++23 concept retained for experimentation and design exploration; not a supported feature-parity implementation | [`libs/cpp/`](./libs/cpp/) | [`updates/implementations/cpp/`](./updates/implementations/cpp/) |

## Project model

- `libs/` contains language-owned source, manifests, tests, and implementation documentation.
- `updates/implementations/<language>/` contains that library's numbered instructions, results, and
  lifecycle status.
- `updates/providers/` contains shared provider contracts and immutable captured evidence.
- `docs/` contains published architecture documentation, decisions, and implementation
  instruction/result mirrors for Mintlify.

Every implementation change uses a pair directly below the library's update directory:

```text
updates/implementations/<language>/NNNN-IMPL-INSTR.md
updates/implementations/<language>/NNNN-IMPL-RESULT.md
```

The instruction defines the bounded objective and inputs. The result records actual changes,
validation, deviations, and a portable semantic change contract. Each library owns its lifecycle
in `STATUS.md`.

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

See the [implementation overview](./updates/README.md),
[implementation workflow](./updates/TEMPLATE_IMPL.md),
[status schema](./updates/TEMPLATE_STATUS.md),
[provider workflow](./updates/TEMPLATE_PROVIDER.md), and
[TypeScript status](./updates/implementations/typescript/STATUS.md).

## Repository layout

```text
libs/
  typescript/
  cpp/

updates/
  TEMPLATE_IMPL.md
  TEMPLATE_PROVIDER.md
  TEMPLATE_STATUS.md
  implementations/
    typescript/
      STATUS.md
      0001-IMPL-INSTR.md
      0001-IMPL-RESULT.md
    cpp/
      STATUS.md
      0001-IMPL-INSTR.md
      0001-IMPL-RESULT.md
  providers/
    <provider>/
      PROVIDER.md
      <snapshot>/
        SNAPSHOT.md
        artifacts/

docs/
  impl/
    typescript/
      0001-IMPL-INSTR.md
      0001-IMPL-RESULT.md
```
