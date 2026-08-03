import assert from "node:assert/strict";
import test from "node:test";

import * as cip from "@xray-network/xray-cardano-lib-cip";
import { AssetFingerprint } from "@xray-network/xray-cardano-lib-cip/cip14";
import { CIP25Metadata } from "@xray-network/xray-cardano-lib-cip/cip25";
import { CIP36KeyRegistration } from "@xray-network/xray-cardano-lib-cip/cip36";
import { CIP4 } from "@xray-network/xray-cardano-lib-cip/cip4";
import { COSESign1 } from "@xray-network/xray-cardano-lib-cip/cip8";

test("CIP package exposes proposal-aligned public namespaces", () => {
  assert.strictEqual(cip.cip14.AssetFingerprint, AssetFingerprint);
  assert.strictEqual(cip.cip25.CIP25Metadata, CIP25Metadata);
  assert.strictEqual(cip.cip36.CIP36KeyRegistration, CIP36KeyRegistration);
  assert.strictEqual(cip.cip4.CIP4, CIP4);
  assert.strictEqual(cip.cip8.COSESign1, COSESign1);
});
