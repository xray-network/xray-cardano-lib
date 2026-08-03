import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import test from "node:test";

import { decodeBech32, encodeBech32 } from "@xray-network/xray-cardano-lib-core/bech32";
import { Credential, GovActionId } from "@xray-network/xray-cardano-lib-chain/conway";
import { Ed25519KeyHash, ScriptHash, TransactionHash } from "@xray-network/xray-cardano-lib-crypto";
import {
  ProvisionalGovernanceActionId,
  ProvisionalGovernanceCredentialId,
  ProvisionalGovernanceCredentialRole,
  decodeLegacyCip105GovernanceCredential,
} from "@xray-network/xray-cardano-lib-cip/cip129";

const toHex = (value) => Buffer.from(value).toString("hex");
const vectorUrls = [1, 2, 3, 4].map((number) => new URL(
  `../../../../../.xray/updates/providers/cardano-cips/0001-cardano-cips/artifacts/upstream/CIP-0105/test-vectors/test-vector-${number}.md`,
  import.meta.url,
));
const roles = {
  cc_hot: ProvisionalGovernanceCredentialRole.ConstitutionalCommitteeHot,
  cc_cold: ProvisionalGovernanceCredentialRole.ConstitutionalCommitteeCold,
  drep: ProvisionalGovernanceCredentialRole.DRep,
};

test("every CIP-0105 key, hash, script, legacy ID, and CIP-0129 assertion decodes to its captured bytes", async () => {
  for (const url of vectorUrls) {
    const source = await readFile(url, "utf8");
    let assertions = 0;
    for (const match of source.matchAll(/Hex: `([0-9a-f]+)`\s+Bech32: `([^`]+)`/gu)) {
      const expectedHex = match[1];
      const encoded = match[2];
      const decoded = decodeBech32(encoded);
      assert.equal(toHex(decoded.bytes), expectedHex);
      assertions += 1;
      if (decoded.prefix in roles && decoded.bytes.length === 29) {
        const identifier = ProvisionalGovernanceCredentialId.from_bech32(encoded);
        assert.equal(identifier.role(), roles[decoded.prefix]);
        assert.equal(identifier.to_bech32(), encoded);
        assert.equal(toHex(identifier.to_raw_bytes()), expectedHex);
        assert.equal((identifier.credential().as_pub_key() ?? identifier.credential().as_script()).to_hex(), expectedHex.slice(2));
      }
      if ((decoded.prefix in roles || decoded.prefix.endsWith("_script")) && decoded.bytes.length === 28) {
        const legacy = decodeLegacyCip105GovernanceCredential(encoded);
        assert.equal((legacy.credential.as_pub_key() ?? legacy.credential.as_script()).to_hex(), expectedHex);
      }
    }
    assert.equal(assertions, 33);
  }
});

test("provisional CIP-0129 credentials enforce all six headers, HRPs, kinds, and lengths", () => {
  const key = Credential.new_pub_key(Ed25519KeyHash.from_hex("00".repeat(28)));
  const script = Credential.new_script(ScriptHash.from_hex("11".repeat(28)));
  for (const role of Object.values(roles)) {
    for (const credential of [key, script]) {
      const identifier = ProvisionalGovernanceCredentialId.from_credential(role, credential);
      const decoded = decodeBech32(identifier.to_bech32());
      assert.equal(decoded.bytes[0] >>> 4, role);
      assert.equal(decoded.bytes[0] & 15, credential.as_pub_key() === undefined ? 3 : 2);
      assert.equal(ProvisionalGovernanceCredentialId.from_bech32(identifier.to_bech32()).credential().kind(), credential.kind());
    }
  }
  assert.equal(ProvisionalGovernanceCredentialId.from_credential(ProvisionalGovernanceCredentialRole.ConstitutionalCommitteeHot, key).to_bech32(), "cc_hot1qgqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqvcdjk7");
  assert.equal(ProvisionalGovernanceCredentialId.from_credential(ProvisionalGovernanceCredentialRole.ConstitutionalCommitteeCold, Credential.new_script(ScriptHash.from_hex("00".repeat(28)))).to_bech32(), "cc_cold1zvqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq6kflvs");
  assert.equal(ProvisionalGovernanceCredentialId.from_credential(ProvisionalGovernanceCredentialRole.DRep, key).to_bech32(), "drep1ygqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq7vlc9n");

  const valid = ProvisionalGovernanceCredentialId.from_credential(ProvisionalGovernanceCredentialRole.DRep, key).to_raw_bytes();
  assert.throws(() => ProvisionalGovernanceCredentialId.from_bech32(encodeBech32("cc_hot", valid)));
  assert.throws(() => ProvisionalGovernanceCredentialId.from_bech32(encodeBech32("drep", valid.slice(1))));
  assert.throws(() => ProvisionalGovernanceCredentialId.from_bech32(encodeBech32("drep", Uint8Array.from([0x24, ...valid.slice(1)]))));
  assert.throws(() => ProvisionalGovernanceCredentialId.from_bech32(encodeBech32("drep", Uint8Array.from([0x32, ...valid.slice(1)]))));
  const defensive = ProvisionalGovernanceCredentialId.from_credential(ProvisionalGovernanceCredentialRole.DRep, key);
  const raw = defensive.to_raw_bytes(); raw.fill(0);
  assert.equal(defensive.to_raw_bytes()[0], 0x22);
});

test("provisional CIP-0129 governance actions use exactly one index byte", () => {
  const zero = ProvisionalGovernanceActionId.from_parts(TransactionHash.from_hex("11".repeat(32)), 0);
  assert.equal(zero.to_bech32(), "gov_action1zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zygsq6dmejn");
  const seventeen = ProvisionalGovernanceActionId.from_parts(TransactionHash.from_hex("00".repeat(32)), 17);
  assert.equal(seventeen.to_bech32(), "gov_action1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqpzklpgpf");
  for (const index of [0, 17, 255]) {
    const value = ProvisionalGovernanceActionId.from_parts(TransactionHash.from_hex("42".repeat(32)), index);
    assert.equal(ProvisionalGovernanceActionId.from_bech32(value.to_bech32()).index(), index);
    assert.equal(value.gov_action_id().index(), index);
  }
  assert.throws(() => ProvisionalGovernanceActionId.from_parts(TransactionHash.from_hex("42".repeat(32)), 256));
  assert.throws(() => ProvisionalGovernanceActionId.from_gov_action_id(GovActionId.new(TransactionHash.from_hex("42".repeat(32)), 256)));
  assert.throws(() => ProvisionalGovernanceActionId.from_bech32(encodeBech32("gov_action", new Uint8Array(32))));
  assert.throws(() => ProvisionalGovernanceActionId.from_bech32(encodeBech32("action", new Uint8Array(33))));
});
