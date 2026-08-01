import assert from "node:assert/strict";
import test from "node:test";

import {
  AlgorithmId,
  CIP8Message,
  COSEKey,
  COSESign1,
  CurveType,
  ECKey,
  HeaderMap,
  Headers,
  KeyType,
  Label,
  ProtectedHeaderMap,
} from "@xray-network/xray-cardano-lib-cip/cip8";
import {
  bytesToHex,
  hexToBytes,
} from "@xray-network/xray-cardano-lib-core";
import { PrivateKey } from "@xray-network/xray-cardano-lib-crypto";

const addressHex = `60${"11".repeat(28)}`;
const payloadHex = "deadbeef";
const privateKey = PrivateKey.from_normal_bytes(new Uint8Array(32).fill(7));
const privateKeyBech32 = privateKey.to_bech32();
const publicKey = privateKey.to_public();
const keyHash = publicKey.hash().to_hex();
const publicKeyBytes = publicKey.to_raw_bytes();
publicKey.dispose();
privateKey.dispose();

function addressNode(value = addressHex) {
  return {
    kind: "bytes",
    value: hexToBytes(value),
    encoding: { kind: "definite", width: 0 },
  };
}

function keyNodeBytes(value = publicKeyBytes) {
  return {
    kind: "bytes",
    value: value.slice(),
    encoding: { kind: "definite", width: 0 },
  };
}

function signedWithProtected(envelope, buildHeaders) {
  const original = COSESign1.from_cbor_hex(envelope.signature);
  const protectedHeaders = HeaderMap.new();
  buildHeaders(protectedHeaders);
  return {
    ...envelope,
    signature: COSESign1.new(
      Headers.new(ProtectedHeaderMap.new(protectedHeaders), original.headers().unprotected()),
      original.payload(),
      original.signature(),
    ).to_cbor_hex(),
  };
}

function envelopeWithKey(envelope, options = {}) {
  const key = COSEKey.new(options.keyType ?? Label.from_key_type(KeyType.OKP));
  if (options.algorithm !== null) {
    key.set_algorithm_id(options.algorithm ?? Label.from_algorithm_id(AlgorithmId.EdDSA));
  }
  if (options.curve !== null) {
    key.set_header(
      Label.from_ec_key(ECKey.CRV),
      options.curve ?? Label.from_curve_type(CurveType.Ed25519).toNode(),
    );
  }
  if (options.publicKey !== null) {
    key.set_header(Label.from_ec_key(ECKey.X), options.publicKey ?? keyNodeBytes());
  }
  return { ...envelope, key: key.to_cbor_hex() };
}

test("CIP8Message signs and verifies the xray-js envelope contract", () => {
  const envelope = CIP8Message.signData(addressHex, payloadHex, privateKeyBech32);
  assert.deepEqual(Object.keys(envelope).sort(), ["key", "signature"]);
  assert.match(envelope.signature, /^[0-9a-f]+$/u);
  assert.match(envelope.key, /^[0-9a-f]+$/u);
  assert.equal(CIP8Message.verifyData(addressHex, keyHash, payloadHex, envelope), true);

  const sign1 = COSESign1.from_cbor_hex(envelope.signature);
  const protectedHeaders = sign1.headers().protected().deserialized_headers();
  assert.equal(protectedHeaders.algorithm_id().as_int().to_str(), String(AlgorithmId.EdDSA));
  assert.equal(
    bytesToHex(protectedHeaders.header(Label.new_text("address")).value),
    addressHex,
  );
  assert.equal(bytesToHex(sign1.payload()), payloadHex);

  const key = COSEKey.from_cbor_hex(envelope.key);
  assert.equal(key.key_type().as_int().to_str(), String(KeyType.OKP));
  assert.equal(key.algorithm_id().as_int().to_str(), String(AlgorithmId.EdDSA));
  assert.equal(key.header(Label.from_ec_key(ECKey.CRV)).value, BigInt(CurveType.Ed25519));
  assert.deepEqual(key.header(Label.from_ec_key(ECKey.X)).value, publicKeyBytes);
});

test("CIP8Message rejects tampering and semantic header mismatches", () => {
  const envelope = CIP8Message.signData(addressHex, payloadHex, privateKeyBech32);
  assert.equal(CIP8Message.verifyData(`60${"22".repeat(28)}`, keyHash, payloadHex, envelope), false);
  assert.equal(CIP8Message.verifyData(addressHex, "00".repeat(28), payloadHex, envelope), false);
  assert.equal(CIP8Message.verifyData(addressHex, keyHash, "00", envelope), false);

  const sign1 = COSESign1.from_cbor_hex(envelope.signature);
  const tamperedSignature = sign1.signature();
  tamperedSignature[0] ^= 0x80;
  const tampered = {
    ...envelope,
    signature: COSESign1.new(
      sign1.headers(),
      sign1.payload(),
      tamperedSignature,
    ).to_cbor_hex(),
  };
  assert.equal(CIP8Message.verifyData(addressHex, keyHash, payloadHex, tampered), false);

  const wrongSignatureAlgorithm = signedWithProtected(envelope, (headers) => {
    headers.set_algorithm_id(Label.from_algorithm_id(AlgorithmId.ChaCha20Poly1305));
    headers.set_header(Label.new_text("address"), addressNode());
  });
  assert.equal(
    CIP8Message.verifyData(addressHex, keyHash, payloadHex, wrongSignatureAlgorithm),
    false,
  );
  assert.equal(CIP8Message.verifyData(
    addressHex,
    keyHash,
    payloadHex,
    envelopeWithKey(envelope, {
      algorithm: Label.from_algorithm_id(AlgorithmId.ChaCha20Poly1305),
    }),
  ), false);
  assert.equal(CIP8Message.verifyData(
    addressHex,
    keyHash,
    payloadHex,
    envelopeWithKey(envelope, { curve: Label.from_curve_type(CurveType.P256).toNode() }),
  ), false);
  assert.equal(CIP8Message.verifyData(
    addressHex,
    keyHash,
    payloadHex,
    envelopeWithKey(envelope, { keyType: Label.from_key_type(KeyType.EC2) }),
  ), false);
});

test("CIP8Message preserves invalid-header and malformed-envelope errors", () => {
  const envelope = CIP8Message.signData(addressHex, payloadHex, privateKeyBech32);

  const textAddress = signedWithProtected(envelope, (headers) => {
    headers.set_algorithm_id(Label.from_algorithm_id(AlgorithmId.EdDSA));
    headers.set_header(Label.new_text("address"), {
      kind: "text",
      value: addressHex,
      encoding: { kind: "definite", width: 0 },
    });
  });
  assert.throws(
    () => CIP8Message.verifyData(addressHex, keyHash, payloadHex, textAddress),
    /No address found in signature\./u,
  );

  const textAlgorithm = signedWithProtected(envelope, (headers) => {
    headers.set_algorithm_id(Label.new_text("EdDSA"));
    headers.set_header(Label.new_text("address"), addressNode());
  });
  assert.throws(
    () => CIP8Message.verifyData(addressHex, keyHash, payloadHex, textAlgorithm),
    /Failed to retrieve Algorithm Id\./u,
  );
  assert.throws(
    () => CIP8Message.verifyData(
      addressHex,
      keyHash,
      payloadHex,
      envelopeWithKey(envelope, { algorithm: null }),
    ),
    /Failed to retrieve Algorithm Id\./u,
  );
  assert.throws(
    () => CIP8Message.verifyData(
      addressHex,
      keyHash,
      payloadHex,
      envelopeWithKey(envelope, { curve: null }),
    ),
    /Failed to retrieve Curve\./u,
  );
  assert.throws(
    () => CIP8Message.verifyData(
      addressHex,
      keyHash,
      payloadHex,
      envelopeWithKey(envelope, { keyType: Label.new_text("OKP") }),
    ),
    /Failed to retrieve Key Type\./u,
  );
  assert.throws(
    () => CIP8Message.verifyData(
      addressHex,
      keyHash,
      payloadHex,
      envelopeWithKey(envelope, { publicKey: null }),
    ),
    /No public key found\./u,
  );

  const sign1 = COSESign1.from_cbor_hex(envelope.signature);
  const missingPayload = {
    ...envelope,
    signature: COSESign1.new(sign1.headers(), undefined, sign1.signature()).to_cbor_hex(),
  };
  assert.throws(
    () => CIP8Message.verifyData(addressHex, keyHash, payloadHex, missingPayload),
    /No payload found\./u,
  );
  const shortSignature = {
    ...envelope,
    signature: COSESign1.new(sign1.headers(), sign1.payload(), new Uint8Array(63)).to_cbor_hex(),
  };
  assert.throws(
    () => CIP8Message.verifyData(addressHex, keyHash, payloadHex, shortSignature),
    /Ed25519Signature.*64/u,
  );
  assert.throws(
    () => CIP8Message.verifyData(addressHex, keyHash, payloadHex, { ...envelope, signature: "00" }),
  );
});
