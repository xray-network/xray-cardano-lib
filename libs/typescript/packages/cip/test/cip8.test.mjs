import assert from "node:assert/strict";
import test from "node:test";

import {
  AlgorithmId,
  COSEKey,
  COSESign1,
  COSESign1Builder,
  COSESignBuilder,
  COSESignature,
  COSESignatures,
  CounterSignature,
  CurveType,
  ECKey,
  EdDSA25519Key,
  HeaderMap,
  Headers,
  Int,
  KeyOperation,
  KeyType,
  Label,
  LabelKind,
  Labels,
  ProtectedHeaderMap,
  SigContext,
  SigStructure,
  SignedMessage,
  SignedMessageKind,
} from "@xray-network/cardano-cip/cip8";
import {
  CBOR_INT_MAX,
  CBOR_INT_MIN,
  bytesToHex,
  decodeCbor,
  encodeCbor,
  hexToBytes,
} from "@xray-network/cardano-core";
import {
  PrivateKey,
  blake2b224,
} from "@xray-network/cardano-crypto";

function intLabel(value) {
  return Label.new_int(Int.new(value));
}

function emptyHeaders() {
  return Headers.new(ProtectedHeaderMap.new_empty(), HeaderMap.new());
}

function cborBytes(value) {
  return {
    kind: "bytes",
    value: value.slice(),
    encoding: { kind: "definite", width: 0 },
  };
}

test("CIP-8 labels, lists, and headers retain types, order, and lossless CBOR", () => {
  const minimum = intLabel(CBOR_INT_MIN);
  const maximum = intLabel(CBOR_INT_MAX);
  assert.equal(minimum.kind(), LabelKind.Int);
  assert.equal(minimum.as_int().to_str(), CBOR_INT_MIN.toString());
  assert.equal(maximum.as_int().to_str(), CBOR_INT_MAX.toString());
  assert.equal(Label.new_text("content").kind(), LabelKind.Text);
  assert.equal(Label.new_text("content").as_text(), "content");
  assert.throws(() => Label.from_cbor_hex("f5"), /integer or text/u);

  const labels = Labels.new();
  labels.add(minimum);
  labels.add(Label.new_text("two"));
  assert.equal(labels.len(), 2);
  assert.equal(labels.get(1).as_text(), "two");
  assert.throws(() => labels.get(2), RangeError);

  const signature = COSESignature.new(emptyHeaders(), Uint8Array.of(0xaa));
  const counter = CounterSignature.new_single(signature);
  const headers = HeaderMap.new();
  headers.set_algorithm_id(Label.from_algorithm_id(AlgorithmId.EdDSA));
  headers.set_criticality(labels);
  headers.set_content_type(Label.new_text("text/plain"));
  headers.set_key_id(Uint8Array.of(1, 2));
  headers.set_init_vector(Uint8Array.of(3, 4));
  headers.set_partial_init_vector(Uint8Array.of(5, 6));
  headers.set_counter_signature(counter);
  headers.set_header(Label.new_text("custom"), { kind: "boolean", value: true });
  headers.set_header(intLabel(-9n), { kind: "null" });

  assert.equal(headers.algorithm_id().as_int().to_str(), "-8");
  assert.equal(headers.criticality().len(), 2);
  assert.equal(headers.content_type().as_text(), "text/plain");
  assert.deepEqual(headers.key_id(), Uint8Array.of(1, 2));
  assert.deepEqual(headers.init_vector(), Uint8Array.of(3, 4));
  assert.deepEqual(headers.partial_init_vector(), Uint8Array.of(5, 6));
  assert.equal(headers.counter_signature().signatures().len(), 1);
  assert.equal(headers.header(Label.new_text("custom")).value, true);
  assert.equal(headers.keys().len(), 9);

  const detached = headers.key_id();
  detached.fill(0xff);
  assert.deepEqual(headers.key_id(), Uint8Array.of(1, 2));

  const noncanonical = hexToBytes("bf18013807ff");
  const decoded = HeaderMap.from_cbor_bytes(noncanonical);
  assert.deepEqual(decoded.to_cbor_bytes(), noncanonical);
  assert.equal(decoded.to_canonical_cbor_hex(), "a10127");
  decoded.set_key_id(Uint8Array.of(0xaa));
  assert.equal(decoded.to_canonical_cbor_hex(), "a201270441aa");

  assert.throws(
    () => HeaderMap.from_cbor_hex("a201010102"),
    /duplicate headermap label/iu,
  );
  assert.throws(
    () => HeaderMap.from_cbor_hex("a10141aa"),
    /integer or text/u,
  );
});

test("protected headers and COSE Sign1 preserve signature-sensitive bytes", () => {
  const protectedBytes = hexToBytes("46bf18013807ff");
  const protectedHeaders = ProtectedHeaderMap.from_cbor_bytes(protectedBytes);
  assert.deepEqual(protectedHeaders.to_cbor_bytes(), protectedBytes);
  assert.equal(protectedHeaders.deserialized_headers().algorithm_id().as_int().to_str(), "-8");
  assert.equal(ProtectedHeaderMap.new_empty().to_cbor_hex(), "40");
  assert.throws(() => ProtectedHeaderMap.from_cbor_hex("4100"), /headermap requires a cbor map/iu);
  assert.throws(() => ProtectedHeaderMap.from_cbor_hex("42a000"), /trailing/iu);

  const noncanonical = hexToBytes("9f40bf6178f5ff41aa41bbff");
  const sign1 = COSESign1.from_cbor_bytes(noncanonical);
  assert.deepEqual(sign1.to_cbor_bytes(), noncanonical);
  assert.equal(sign1.to_canonical_cbor_hex(), "8440a16178f541aa41bb");
  assert.deepEqual(sign1.payload(), Uint8Array.of(0xaa));
  assert.deepEqual(sign1.signature(), Uint8Array.of(0xbb));

  const payload = sign1.payload();
  payload.fill(0);
  assert.deepEqual(sign1.payload(), Uint8Array.of(0xaa));
  assert.throws(() => COSESign1.from_cbor_hex(`${sign1.to_cbor_hex()}00`), /trailing/iu);
  assert.throws(() => COSESign1.from_cbor_hex("8340a040"), /4-element/u);
  assert.throws(() => COSESign1.from_cbor_hex("8440a0f54100"), /payload requires a cbor byte string/iu);
});

test("signature structures encode exact contexts and optional signer protection", () => {
  const signature1 = SigStructure.new(
    SigContext.Signature1,
    ProtectedHeaderMap.new_empty(),
    Uint8Array.of(1),
    Uint8Array.of(2),
  );
  assert.equal(
    signature1.to_cbor_hex(),
    "846a5369676e6174757265314041014102",
  );
  assert.equal(SigStructure.from_cbor_bytes(signature1.to_cbor_bytes()).context(), SigContext.Signature1);

  const signature = SigStructure.new(
    SigContext.Signature,
    ProtectedHeaderMap.new_empty(),
    Uint8Array.of(1),
    Uint8Array.of(2),
  );
  signature.set_sign_protected(ProtectedHeaderMap.new_empty());
  assert.equal(
    signature.to_cbor_hex(),
    "85695369676e6174757265404041014102",
  );
  assert.equal(signature.sign_protected().to_cbor_hex(), "40");

  const counter = SigStructure.new(
    SigContext.CounterSignature,
    ProtectedHeaderMap.new_empty(),
    new Uint8Array(),
    Uint8Array.of(3),
  );
  assert.equal(counter.context(), SigContext.CounterSignature);
  assert.throws(
    () => SigStructure.from_cbor_bytes(encodeCbor({
      kind: "array",
      values: [
        { kind: "text", value: "wrong", encoding: { kind: "definite", width: 0 } },
        cborBytes(new Uint8Array()),
        cborBytes(new Uint8Array()),
        cborBytes(new Uint8Array()),
      ],
      encoding: { kind: "definite", width: 0 },
    })),
    /unknown signature context/iu,
  );
});

test("the captured EMURGO Sign1 example signs and verifies the SigStructure", () => {
  const privateKey = PrivateKey.from_normal_bytes(Uint8Array.from([
    34, 125, 55, 10, 222, 244, 31, 91, 181, 231, 62, 80, 90, 53, 246, 160,
    226, 111, 123, 228, 188, 90, 15, 130, 210, 206, 78, 199, 209, 18, 202, 234,
  ]));
  const payload = new TextEncoder().encode("message to sign");
  const externalAad = new TextEncoder().encode("externally supplied data not in sign object");
  const builder = COSESign1Builder.new(emptyHeaders(), payload, false);
  builder.set_external_aad(externalAad);

  const dataToSign = builder.make_data_to_sign().to_cbor_bytes();
  const signature = privateKey.sign(dataToSign);
  const sign1 = builder.build(signature.to_raw_bytes());
  const reconstructed = sign1.signed_data(externalAad).to_cbor_bytes();

  assert.deepEqual(reconstructed, dataToSign);
  assert.equal(privateKey.to_public().verify(reconstructed, signature), true);
  assert.deepEqual(sign1.payload(), payload);
  assert.equal(sign1.headers().unprotected().header(Label.new_text("hashed")).value, false);

  const detachedBuilder = COSESign1Builder.new(emptyHeaders(), payload, true);
  const detached = detachedBuilder.build(new Uint8Array(64));
  assert.equal(detached.payload(), undefined);
  assert.throws(() => detached.signed_data(), /no external payload/u);
  assert.deepEqual(detached.signed_data(undefined, payload).payload(), payload);
});

test("Sign and Sign1 builders hash once and construct their distinct signature shapes", () => {
  const payload = Uint8Array.of(1, 2, 3, 4);
  const expectedHash = blake2b224(payload);
  const sign1Builder = COSESign1Builder.new(emptyHeaders(), payload, false);
  sign1Builder.hash_payload();
  const once = sign1Builder.make_data_to_sign().to_cbor_bytes();
  sign1Builder.hash_payload();
  assert.deepEqual(sign1Builder.make_data_to_sign().to_cbor_bytes(), once);
  assert.deepEqual(sign1Builder.make_data_to_sign().payload(), expectedHash);
  assert.equal(
    sign1Builder.build(new Uint8Array(64)).headers().unprotected().header(Label.new_text("hashed")).value,
    true,
  );

  const signatureHeaders = emptyHeaders();
  const signatures = COSESignatures.new();
  signatures.add(COSESignature.new(signatureHeaders, Uint8Array.of(9)));
  signatures.add(COSESignature.new(signatureHeaders, Uint8Array.of(10)));
  const signBuilder = COSESignBuilder.new(emptyHeaders(), payload, false);
  signBuilder.hash_payload();
  signBuilder.hash_payload();
  assert.equal(signBuilder.make_data_to_sign().context(), SigContext.Signature);
  assert.deepEqual(signBuilder.make_data_to_sign().payload(), expectedHash);
  const sign = signBuilder.build(signatures);
  assert.equal(sign.signatures().len(), 2);
  assert.equal(sign.headers().unprotected().header(Label.new_text("hashed")), undefined);
  assert.throws(() => sign.signatures().get(2), RangeError);

  const single = CounterSignature.new_single(signatures.get(0));
  assert.equal(decodeCbor(single.to_cbor_bytes()).values.length, 3);
  const multi = CounterSignature.new_multi(signatures);
  assert.equal(CounterSignature.from_cbor_bytes(multi.to_cbor_bytes()).signatures().len(), 2);
});

test("public-only Ed25519 COSE keys use the captured labels and reject malformed maps", () => {
  const publicKey = Uint8Array.from({ length: 32 }, (_, index) => index);
  const builder = EdDSA25519Key.new(publicKey);
  builder.is_for_signing();
  builder.is_for_verifying();
  const key = builder.build();

  assert.equal(key.key_type().as_int().to_str(), String(KeyType.OKP));
  assert.equal(key.algorithm_id().as_int().to_str(), String(AlgorithmId.EdDSA));
  assert.equal(
    key.header(Label.from_ec_key(ECKey.CRV)).value,
    BigInt(CurveType.Ed25519),
  );
  assert.deepEqual(key.header(Label.from_ec_key(ECKey.X)).value, publicKey);
  assert.equal(key.key_ops().get(0).as_int().to_str(), String(KeyOperation.Sign));
  assert.equal(key.key_ops().get(1).as_int().to_str(), String(KeyOperation.Verify));
  assert.equal(key.header(Label.from_ec_key(ECKey.D)), undefined);
  assert.equal("set_private_key" in builder, false);

  const expected = `a501012006215820${bytesToHex(publicKey)}032704820102`;
  assert.equal(key.to_cbor_hex(), expected);
  assert.equal(COSEKey.from_cbor_bytes(key.to_cbor_bytes()).to_cbor_hex(), expected);
  assert.throws(() => EdDSA25519Key.new(new Uint8Array(31)), /32/u);
  assert.throws(() => COSEKey.from_cbor_hex("a0"), /missing required key type/u);
  assert.throws(() => COSEKey.from_cbor_hex("a201010102"), /duplicate cosekey label/iu);
});

test("SignedMessage matches the captured cms_ vector and padded variants", () => {
  const pad1 = "cms_hEChAzkD51gnQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQwECZACyaZmw==";
  const pad2 = "cms_hEChAzkD51gnQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQwECZA==CyaZmw";
  const pad3 = "cms_hEChAzkD51gnQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQwECZA==CyaZmw==";
  const message1 = SignedMessage.from_user_facing_encoding(pad1);
  const message2 = SignedMessage.from_user_facing_encoding(pad2);
  const message3 = SignedMessage.from_user_facing_encoding(pad3);

  assert.equal(message1.kind(), SignedMessageKind.COSESIGN1);
  assert.deepEqual(message1.to_cbor_bytes(), message2.to_cbor_bytes());
  assert.deepEqual(message2.to_cbor_bytes(), message3.to_cbor_bytes());
  assert.equal(
    SignedMessage.from_user_facing_encoding(message1.to_user_facing_encoding()).to_cbor_hex(),
    message1.to_cbor_hex(),
  );
  assert.equal(message1.as_cose_sign(), undefined);
  assert.equal(message1.as_cose_sign1().payload().length, 39);

  assert.throws(() => SignedMessage.from_user_facing_encoding("bad_value"), /cms_/u);
  assert.throws(() => SignedMessage.from_user_facing_encoding("cms_a"), /checksum/u);
  assert.throws(
    () => SignedMessage.from_user_facing_encoding(`${message1.to_user_facing_encoding().slice(0, -1)}A`),
    /checksum/u,
  );

  const signatures = COSESignatures.new();
  const signed = COSESignBuilder.new(emptyHeaders(), Uint8Array.of(1), false).build(signatures);
  const wrapped = SignedMessage.new_cose_sign(signed);
  assert.equal(wrapped.kind(), SignedMessageKind.COSESIGN);
  assert.equal(SignedMessage.from_cbor_bytes(wrapped.to_cbor_bytes()).as_cose_sign().signatures().len(), 0);
});

test("CIP-8 decoders reject malformed fixed fields and preserve defensive ownership", () => {
  assert.throws(() => COSESignature.from_cbor_hex("8240a0"), /3-element/u);
  assert.throws(() => COSESignatures.from_cbor_hex("40"), /array/u);
  assert.throws(() => Headers.from_cbor_hex("8140"), /2-element/u);
  assert.throws(() => ProtectedHeaderMap.from_cbor_hex("5f6100ff"), /invalid chunk|headermap/iu);

  const signatureBytes = new Uint8Array(64).fill(7);
  const signature = COSESignature.new(emptyHeaders(), signatureBytes);
  signatureBytes.fill(0);
  assert.deepEqual(signature.signature(), new Uint8Array(64).fill(7));
  const returned = signature.signature();
  returned.fill(1);
  assert.deepEqual(signature.signature(), new Uint8Array(64).fill(7));
});
