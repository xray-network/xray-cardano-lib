import assert from "node:assert/strict";
import test from "node:test";

import * as cip from "@xray-network/xray-cardano-lib-cip";
import { CIP4 } from "@xray-network/xray-cardano-lib-cip/cip4";
import { COSESign1 } from "@xray-network/xray-cardano-lib-cip/cip8";
import { AssetFingerprint } from "@xray-network/xray-cardano-lib-cip/cip14";
import { diagnose_cip21_transaction } from "@xray-network/xray-cardano-lib-cip/cip21";
import { CIP25Metadata } from "@xray-network/xray-cardano-lib-cip/cip25";
import { CIP36KeyRegistration } from "@xray-network/xray-cardano-lib-cip/cip36";
import { encode_asset_name_label } from "@xray-network/xray-cardano-lib-cip/cip67";
import { CIP68Datum } from "@xray-network/xray-cardano-lib-cip/cip68";

test("CIP package exposes proposal-aligned public namespaces", () => {
  assert.strictEqual(cip.cip14.AssetFingerprint, AssetFingerprint);
  assert.strictEqual(cip.cip21.diagnose_cip21_transaction, diagnose_cip21_transaction);
  assert.strictEqual(cip.cip25.CIP25Metadata, CIP25Metadata);
  assert.strictEqual(cip.cip36.CIP36KeyRegistration, CIP36KeyRegistration);
  assert.strictEqual(cip.cip67.encode_asset_name_label, encode_asset_name_label);
  assert.strictEqual(cip.cip68.CIP68Datum, CIP68Datum);
  assert.strictEqual(cip.cip4.CIP4, CIP4);
  assert.strictEqual(cip.cip8.COSESign1, COSESign1);
  assert.equal("cip129" in cip, false);
});
