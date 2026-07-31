import assert from "node:assert/strict";
import test from "node:test";

import * as cip from "@xray-network/cardano-cip";
import { CIP25Metadata } from "@xray-network/cardano-cip/cip25";
import { CIP36KeyRegistration } from "@xray-network/cardano-cip/cip36";
import { COSESign1 } from "@xray-network/cardano-cip/cip8";

test("CIP package exposes proposal-aligned public namespaces", () => {
  assert.strictEqual(cip.cip25.CIP25Metadata, CIP25Metadata);
  assert.strictEqual(cip.cip36.CIP36KeyRegistration, CIP36KeyRegistration);
  assert.strictEqual(cip.cip8.COSESign1, COSESign1);
});
