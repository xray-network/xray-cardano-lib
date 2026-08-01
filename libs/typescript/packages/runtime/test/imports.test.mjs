import assert from "node:assert/strict";
import test from "node:test";

import * as cardano from "@xray-network/xray-cardano-lib";
import {
  CIP8Message as AggregateCIP8Message,
  Constr as AggregateConstr,
  Data as AggregateData,
} from "@xray-network/xray-cardano-lib";
import * as chain from "@xray-network/xray-cardano-lib-chain";
import * as allegra from "@xray-network/xray-cardano-lib-chain/allegra";
import * as alonzo from "@xray-network/xray-cardano-lib-chain/alonzo";
import * as babbage from "@xray-network/xray-cardano-lib-chain/babbage";
import * as byron from "@xray-network/xray-cardano-lib-chain/byron";
import * as conway from "@xray-network/xray-cardano-lib-chain/conway";
import * as mary from "@xray-network/xray-cardano-lib-chain/mary";
import * as multiEra from "@xray-network/xray-cardano-lib-chain/multi-era";
import * as shelley from "@xray-network/xray-cardano-lib-chain/shelley";
import * as cip from "@xray-network/xray-cardano-lib-cip";
import * as cip25 from "@xray-network/xray-cardano-lib-cip/cip25";
import * as cip36 from "@xray-network/xray-cardano-lib-cip/cip36";
import * as cip4 from "@xray-network/xray-cardano-lib-cip/cip4";
import * as cip8 from "@xray-network/xray-cardano-lib-cip/cip8";
import * as core from "@xray-network/xray-cardano-lib-core";
import * as crypto from "@xray-network/xray-cardano-lib-crypto";
import * as plutus from "@xray-network/xray-cardano-lib-plutus";
import * as plutusData from "@xray-network/xray-cardano-lib-plutus/data";
import * as uplc from "@xray-network/xray-cardano-lib-plutus/uplc";

test("public packages share each nominal class owner", () => {
  assert.strictEqual(cardano.CardanoError, core.CardanoError);
  assert.strictEqual(cardano.TransactionHash, crypto.TransactionHash);
  assert.strictEqual(cardano.TransactionInput, chain.TransactionInput);
  assert.strictEqual(multiEra.MultiEraBlock, chain.MultiEraBlock);
  assert.strictEqual(byron.ByronBlock, chain.ByronBlock);
  assert.strictEqual(shelley.ShelleyBlock, chain.ShelleyBlock);
  assert.strictEqual(allegra.AllegraBlock, chain.AllegraBlock);
  assert.strictEqual(mary.MaryBlock, chain.MaryBlock);
  assert.strictEqual(alonzo.AlonzoBlock, chain.AlonzoBlock);
  assert.strictEqual(babbage.BabbageTransactionBody, chain.BabbageTransactionBody);
  assert.strictEqual(conway.Block, chain.Block);
  assert.strictEqual(cardano.CIP25Metadata, cip25.CIP25Metadata);
  assert.strictEqual(cardano.CIP36KeyRegistration, cip36.CIP36KeyRegistration);
  assert.strictEqual(cardano.CIP4, cip4.CIP4);
  assert.strictEqual(cardano.COSESign1, cip8.COSESign1);
  assert.strictEqual(AggregateCIP8Message, cip8.CIP8Message);
  assert.strictEqual(cip.cip25.CIP25Metadata, cip25.CIP25Metadata);
  assert.strictEqual(cip.cip4.CIP4, cip4.CIP4);
  assert.strictEqual(cip.cip8.COSESign1, cip8.COSESign1);
  assert.strictEqual(cip.cip8.CIP8Message, cip8.CIP8Message);
  assert.strictEqual(plutus.Data, plutusData.Data);
  assert.strictEqual(AggregateData, plutusData.Data);
  assert.strictEqual(AggregateConstr, plutusData.Constr);
  assert.strictEqual(cip8.Int, core.Int);
  assert.strictEqual(cip8.PublicKey, crypto.PublicKey);
  assert.ok(new cardano.CardanoError("INVARIANT", "identity") instanceof core.CardanoError);
});

test("all six package entry points and focused subpaths expose their primary APIs", () => {
  assert.equal(typeof core.CardanoError, "function");
  assert.equal(typeof crypto.secureRandomBytes, "function");
  assert.equal(typeof chain.TransactionInput, "function");
  assert.equal(typeof byron.ByronBlock, "function");
  assert.equal(typeof shelley.ShelleyBlock, "function");
  assert.equal(typeof allegra.AllegraBlock, "function");
  assert.equal(typeof mary.MaryBlock, "function");
  assert.equal(typeof alonzo.AlonzoBlock, "function");
  assert.equal(typeof babbage.BabbageTransactionBody, "function");
  assert.equal(typeof conway.Block, "function");
  assert.equal("CONWAY_RULES" in conway, false);
  assert.equal("validateConwayRule" in conway, false);
  assert.equal("GeneratedFixedMapProbe" in conway, false);
  assert.equal("TaggedTransactionInputSet" in conway, false);
  assert.strictEqual(cardano.applyParamsToScript, plutus.applyParamsToScript);
  assert.strictEqual(cardano.evaluatePhaseTwoRaw, plutus.evaluatePhaseTwoRaw);
  assert.equal("apply_params_to_script" in cardano, false);
  assert.equal("eval_phase_two_raw" in cardano, false);
  assert.equal("apply_params_to_script" in plutus, false);
  assert.equal("eval_phase_two_raw" in plutus, false);
  assert.strictEqual(plutus.parseUplcText, uplc.parseUplcText);
  assert.strictEqual(plutus.decodeFlatProgram, uplc.decodeFlatProgram);
  assert.strictEqual(plutus.evaluateProgram, uplc.evaluateProgram);
  assert.strictEqual(cardano.parseUplcText, uplc.parseUplcText);
  assert.strictEqual(cardano.evaluateProgram, uplc.evaluateProgram);
  assert.equal(AggregateData.to(undefined, AggregateData.Void()), "d87980");
  assert.ok(new AggregateConstr(0, []) instanceof plutusData.Constr);
  assert.equal(typeof plutus.Constr, "function");
  assert.equal("PlutusData" in plutus, false);
  assert.equal("parsePlutusData" in plutus, false);
  assert.equal("parsePlutusDataHex" in plutus, false);
  assert.equal(typeof cip25.CIP25Metadata, "function");
  assert.equal(typeof cip36.CIP36KeyRegistration, "function");
  assert.equal(typeof cip4.CIP4.calculateChecksum, "function");
  assert.equal(typeof cip8.COSESign1Builder, "function");
  assert.equal(typeof AggregateCIP8Message.signData, "function");
  assert.equal(typeof AggregateCIP8Message.verifyData, "function");
  assert.equal(typeof multiEra.MultiEraBlock, "function");
  assert.equal(typeof cardano.TransactionBuilder, "function");
});
