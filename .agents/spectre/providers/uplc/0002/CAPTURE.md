# uplc capture 0002 summary

Provider-Capture-Version: v1
Provider: uplc
Capture: 0002
Snapshot: SNAPSHOT.md
Previous-Capture: 0001/SNAPSHOT.md#change-summary

## Summary

This summary retains the original upstream review findings. The human-requested development
reconciliation changes storage and metadata only, not upstream revisions or runtime behavior.
SNAPSHOT.md is the sole authority for specification, inventories, counts and removals.

## Change summary

The selected Plutus runner, availability, costs and Ledger integration evidence changed;
unchanged sources and licenses are inherited directly from 0001. See the authoritative
[artifact comparison](SNAPSHOT.md#changes-from-predecessor) for exact storage accounting.
The corpus contains added, changed and removed paths. Path-level changes, including the shared
budget migration, are not automatically semantic additions/removals; see SNAPSHOT.md for counts.

| Change | Captured evidence | Observation and significance |
| --- | --- | --- |
| Text/Flat corpus layout | [runner conventions](artifacts/plutus/plutus-conformance/src/PlutusConformance/Common.hs), [upstream README](artifacts/plutus/plutus-conformance/README.md), [corpus](artifacts/conformance/corpus.json) | Binary Flat input/results, shared budget names and parse/decode markers; all entries/associations are retained. Two non-Markdown auxiliaries expose a remaining TS reader gap. |
| Future builtin definitions | [Builtins.hs](artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Default/Builtins.hs), [Versions.hs](artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/Versions.hs) | MultiIndexArray, Policies and AssetCount are batch7 at futurePV, not enabled for maintained protocols 5–11. |
| Cost and parameter additions | [cost-model data](artifacts/plutus/plutus-core/cost-model/data/builtinCostModelE.json), [V3 parameter names](artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/ParamName.hs) | Every A–E builtin JSON adds multiIndexArray; all pre-existing entries are unchanged. Parameter enums append five future-builtin parameters. All A–E CEK machine-cost JSON files are byte-identical. No existing-cost rewrite is indicated by these data diffs. |
| V4/protocol 12 | [ProtocolVersions.hs](artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/ProtocolVersions.hs), [Versions.hs](artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/Versions.hs) | Introduces Dijkstra/V4 availability metadata; outside maintained consumer scope. |
| Host-language refactors | [ExBudget.hs](artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExBudget.hs), [CekMachineCosts.hs](artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/CekMachineCosts.hs) | Haskell JSON derivation replaced by explicit instances with equivalent field-label options; not by itself evidence for a TS behavior change. |
| Ledger integration | [Alonzo collection/evaluation](artifacts/cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Plutus/Evaluate.hs), [Conway contexts](artifacts/cardano-ledger/eras/conway/impl/src/Cardano/Ledger/Conway/TxInfo.hs), [core evaluation](artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/Evaluate.hs) | Script preparation/context representation and newer-language integration changed. Full in-scope behavioral mapping remains to be established; no unverified TS defect is asserted. |

## Consumer impact and recommended work

Paths below are repository-root-relative. Existing LOCAL work is compared as current owned code;
it is not retroactively declared DIRECT, accepted, or proven equivalent to all new upstream evidence.

| Finding | Maintained target / modules and APIs | Tests and validation | Recommendation |
| --- | --- | --- | --- |
| Complete auxiliary accounting | TypeScript: libs/typescript/packages/plutus/test/support/conformance.mjs, readConformanceCorpus | conformance-layout.test.mjs and conformance.test.mjs in the same test directory | Plan explicit safe, exact non-Markdown auxiliary paths; retain corpus bytes and never execute ren.sh. Existing 0029 transport support is useful but does not yet accept this complete snapshot. |
| Adopt a second pinned corpus | Same test owners; readConformanceCorpus/runConformanceCorpus | Assert 4,822 entries, 1,905 format cases, 8 auxiliaries and all associations; preserve the 3,013/1,003 old suite | Use this immutable snapshot path in a separate adoption plan after resolving scope/failures. Keep text and Flat result formats distinct and historical/shared budget checks strict. |
| Runtime comparison pending | libs/typescript/packages/plutus/src/uplc/flat.ts, text.ts, machine.ts | Flat/text/machine/conformance tests | Compare each in-scope case and classify every mismatch before proposing runtime changes. A capture or harness-only LOCAL fix does not prove 1.68 compatibility. |
| Existing costs unchanged; future entries added | libs/typescript/packages/plutus/src/uplc/cost-model.ts and cost-model-data.ts | Cost-model tests; protocol/language gating | No automatic update to existing default cost entries. Review parameter-tail compatibility explicitly; do not add future-gated builtin semantics merely because captured costs exist. |
| Phase-two contexts/script preparation | libs/typescript/packages/plutus/src/ledger/evaluate.ts and context.ts | ledger.test.mjs and api.test.mjs | Investigate the 18 changed Ledger sources across Alonzo 5–6, Babbage 7–8 and Conway 9–11: script resolution, datums/arguments, redeemer ordering, cost models, independent budgets and CPU/memory order. Create bounded implementation work only for demonstrated differences. |
| Future functionality | Same UPLC/Ledger owners | Availability tests | Plutus V4, protocol 12, Dijkstra and batch7 remain out of scope. Retain their evidence and explicit availability rules; no automatic C++ work. |

## Semantic evidence

Source comparison covers every selected file by bytes. Focused semantic review read runner/format
rules, builtin and protocol-availability diffs, cost-model data, parameter tails, JSON refactors and
selected Ledger script/context changes. It does not claim complete Haskell behavioral equivalence
or conformance execution. The following exact changed-source inventory preserves the review scope.

- [cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Plutus/Context.hs](artifacts/cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Plutus/Context.hs)
- [cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Plutus/Evaluate.hs](artifacts/cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Plutus/Evaluate.hs)
- [cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Plutus/TxInfo.hs](artifacts/cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Plutus/TxInfo.hs)
- [cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Rules/Utxos.hs](artifacts/cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Rules/Utxos.hs)
- [cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Scripts.hs](artifacts/cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Scripts.hs)
- [cardano-ledger/eras/alonzo/impl/test/Test/Cardano/Ledger/Alonzo/Imp/TxInfoSpec.hs](artifacts/cardano-ledger/eras/alonzo/impl/test/Test/Cardano/Ledger/Alonzo/Imp/TxInfoSpec.hs)
- [cardano-ledger/eras/babbage/impl/src/Cardano/Ledger/Babbage/Rules/Utxos.hs](artifacts/cardano-ledger/eras/babbage/impl/src/Cardano/Ledger/Babbage/Rules/Utxos.hs)
- [cardano-ledger/eras/babbage/impl/src/Cardano/Ledger/Babbage/TxInfo.hs](artifacts/cardano-ledger/eras/babbage/impl/src/Cardano/Ledger/Babbage/TxInfo.hs)
- [cardano-ledger/eras/babbage/impl/testlib/Test/Cardano/Ledger/Babbage/TxInfoSpec.hs](artifacts/cardano-ledger/eras/babbage/impl/testlib/Test/Cardano/Ledger/Babbage/TxInfoSpec.hs)
- [cardano-ledger/eras/conway/impl/src/Cardano/Ledger/Conway/Rules/Utxos.hs](artifacts/cardano-ledger/eras/conway/impl/src/Cardano/Ledger/Conway/Rules/Utxos.hs)
- [cardano-ledger/eras/conway/impl/src/Cardano/Ledger/Conway/Scripts.hs](artifacts/cardano-ledger/eras/conway/impl/src/Cardano/Ledger/Conway/Scripts.hs)
- [cardano-ledger/eras/conway/impl/src/Cardano/Ledger/Conway/TxInfo.hs](artifacts/cardano-ledger/eras/conway/impl/src/Cardano/Ledger/Conway/TxInfo.hs)
- [cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/Data.hs](artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/Data.hs)
- [cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/Evaluate.hs](artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/Evaluate.hs)
- [cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/ExUnits.hs](artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/ExUnits.hs)
- [cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/Language.hs](artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/Language.hs)
- [cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/ToPlutusData.hs](artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/ToPlutusData.hs)
- [cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/TxInfo.hs](artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/TxInfo.hs)
- [plutus/plutus-conformance/README.md](artifacts/plutus/plutus-conformance/README.md)
- [plutus/plutus-conformance/src/PlutusConformance/Common.hs](artifacts/plutus/plutus-conformance/src/PlutusConformance/Common.hs)
- [plutus/plutus-core/cost-model/data/builtinCostModelA.json](artifacts/plutus/plutus-core/cost-model/data/builtinCostModelA.json)
- [plutus/plutus-core/cost-model/data/builtinCostModelB.json](artifacts/plutus/plutus-core/cost-model/data/builtinCostModelB.json)
- [plutus/plutus-core/cost-model/data/builtinCostModelC.json](artifacts/plutus/plutus-core/cost-model/data/builtinCostModelC.json)
- [plutus/plutus-core/cost-model/data/builtinCostModelD.json](artifacts/plutus/plutus-core/cost-model/data/builtinCostModelD.json)
- [plutus/plutus-core/cost-model/data/builtinCostModelE.json](artifacts/plutus/plutus-core/cost-model/data/builtinCostModelE.json)
- [plutus/plutus-core/plutus-core/src/PlutusCore/Default/Builtins.hs](artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Default/Builtins.hs)
- [plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/BuiltinCostModel.hs](artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/BuiltinCostModel.hs)
- [plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/CostingFun/Core.hs](artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/CostingFun/Core.hs)
- [plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExBudget.hs](artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExBudget.hs)
- [plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExBudgetingDefaults.hs](artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExBudgetingDefaults.hs)
- [plutus/plutus-core/plutus-core/src/PlutusCore/Value.hs](artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Value.hs)
- [plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/CekMachineCosts.hs](artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/CekMachineCosts.hs)
- [plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/ProtocolVersions.hs](artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/ProtocolVersions.hs)
- [plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/Versions.hs](artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/Versions.hs)
- [plutus/plutus-ledger-api/src/PlutusLedgerApi/Data/V1.hs](artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Data/V1.hs)
- [plutus/plutus-ledger-api/src/PlutusLedgerApi/Data/V2.hs](artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Data/V2.hs)
- [plutus/plutus-ledger-api/src/PlutusLedgerApi/Data/V3.hs](artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Data/V3.hs)
- [plutus/plutus-ledger-api/src/PlutusLedgerApi/MachineParameters.hs](artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/MachineParameters.hs)
- [plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Value.hs](artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Value.hs)
- [plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/ParamName.hs](artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/ParamName.hs)
- [plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Value.hs](artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Value.hs)
- [plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/ParamName.hs](artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/ParamName.hs)
- [plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/ParamName.hs](artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/ParamName.hs)

## Unresolved questions

- Resolve the reader’s non-Markdown auxiliary handling before adopting the entire corpus.
- Classify every new-corpus case’s supported availability and run it through owned code in a later
  consumer plan; do not derive exclusions solely from failing tests or directory names.
- Establish whether the changed Ledger preparation/context paths alter any maintained observable
  behavior before changing evaluate.ts/context.ts; distinguish newer-language work and refactors.
- The exact standalone source selection is bounded evidence, not a complete future-feature source
  closure. New dependencies such as PlutusCore.Arrays are not captured and do not authorize batch7.

## Exclusions

Upstream code/tool execution, source generation, automatic adoption, full phase-one validation,
third-party normative implementations, new runtime support beyond protocols 5–11, scheduling and
GitHub Actions. Auxiliary tooling remains inert base64 evidence. Old snapshots and active REVIEW
implementation records remain unchanged.

