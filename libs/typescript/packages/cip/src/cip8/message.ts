import {
  bytesToHex,
  hexToBytes,
} from "@xray-network/xray-cardano-lib-core";
import {
  Ed25519Signature,
  PrivateKey,
  PublicKey,
} from "@xray-network/xray-cardano-lib-crypto";

import { COSESign1Builder } from "./builders.js";
import {
  AlgorithmId,
  COSEKey,
  COSESign1,
  CurveType,
  ECKey,
  HeaderMap,
  Headers,
  KeyType,
  Label,
  ProtectedHeaderMap,
} from "./model.js";

export interface CIP8MessageEnvelope {
  readonly signature: string;
  readonly key: string;
}

function signData(
  addressHex: string,
  payloadHex: string,
  privateKeyBech32: string,
): CIP8MessageEnvelope {
  const protectedHeaders = HeaderMap.new();
  protectedHeaders.set_algorithm_id(Label.from_algorithm_id(AlgorithmId.EdDSA));
  protectedHeaders.set_header(Label.new_text("address"), {
    kind: "bytes",
    value: hexToBytes(addressHex),
    encoding: { kind: "definite", width: 0 },
  });
  const headers = Headers.new(
    ProtectedHeaderMap.new(protectedHeaders),
    HeaderMap.new(),
  );
  const builder = COSESign1Builder.new(headers, hexToBytes(payloadHex), false);
  const privateKey = PrivateKey.from_bech32(privateKeyBech32);
  try {
    const signature = privateKey.sign(builder.make_data_to_sign().to_cbor_bytes());
    let signatureBytes: Uint8Array;
    try {
      signatureBytes = signature.to_raw_bytes();
    } finally {
      signature.dispose();
    }
    const coseSign1 = builder.build(signatureBytes);

    const publicKey = privateKey.to_public();
    let publicKeyBytes: Uint8Array;
    try {
      publicKeyBytes = publicKey.to_raw_bytes();
    } finally {
      publicKey.dispose();
    }
    const key = COSEKey.new(Label.from_key_type(KeyType.OKP));
    key.set_algorithm_id(Label.from_algorithm_id(AlgorithmId.EdDSA));
    key.set_header(
      Label.from_ec_key(ECKey.CRV),
      Label.from_curve_type(CurveType.Ed25519).toNode(),
    );
    key.set_header(Label.from_ec_key(ECKey.X), {
      kind: "bytes",
      value: publicKeyBytes,
      encoding: { kind: "definite", width: 0 },
    });

    return {
      signature: coseSign1.to_cbor_hex(),
      key: key.to_cbor_hex(),
    };
  } finally {
    privateKey.dispose();
  }
}

function verifyData(
  addressHex: string,
  keyHash: string,
  payloadHex: string,
  signedMessage: CIP8MessageEnvelope,
): boolean {
  const coseSign1 = COSESign1.from_cbor_hex(signedMessage.signature);
  const key = COSEKey.from_cbor_hex(signedMessage.key);
  const protectedHeaders = coseSign1.headers().protected().deserialized_headers();

  const signedAddress = readAddress(protectedHeaders);
  const signatureAlgorithm = readAlgorithm(
    protectedHeaders.algorithm_id(),
    "Failed to retrieve Algorithm Id.",
  );
  const keyAlgorithm = readAlgorithm(
    key.algorithm_id(),
    "Failed to retrieve Algorithm Id.",
  );
  const keyCurve = readCurve(key);
  const keyType = readKeyType(key);
  const publicKey = readPublicKey(key);
  try {
    const signedPayload = readPayload(coseSign1);
    const signature = Ed25519Signature.from_raw_bytes(coseSign1.signature());
    try {
      const signedData = coseSign1.signed_data(undefined, undefined).to_cbor_bytes();
      if (signedAddress !== addressHex) return false;
      if (keyHash !== publicKey.hash().to_hex()) return false;
      if (signatureAlgorithm !== AlgorithmId.EdDSA || keyAlgorithm !== AlgorithmId.EdDSA) {
        return false;
      }
      if (keyCurve !== CurveType.Ed25519) return false;
      if (keyType !== KeyType.OKP) return false;
      if (signedPayload !== payloadHex) return false;
      return publicKey.verify(signedData, signature);
    } finally {
      signature.dispose();
    }
  } finally {
    publicKey.dispose();
  }
}

function readAddress(headers: HeaderMap): string {
  try {
    const value = headers.header(Label.new_text("address"));
    if (value?.kind !== "bytes") throw new TypeError("Address header is not bytes");
    return bytesToHex(value.value);
  } catch {
    throw new Error("No address found in signature.");
  }
}

function readAlgorithm(value: Label | undefined, errorMessage: string): number {
  try {
    const integer = value?.as_int();
    if (integer === undefined) throw new TypeError("Algorithm Id is not an integer");
    return Number(integer.to_str());
  } catch {
    throw new Error(errorMessage);
  }
}

function readCurve(key: COSEKey): number {
  try {
    const value = key.header(Label.from_ec_key(ECKey.CRV));
    if (value?.kind !== "unsigned" && value?.kind !== "negative") {
      throw new TypeError("Curve is not an integer");
    }
    return Number(value.value);
  } catch {
    throw new Error("Failed to retrieve Curve.");
  }
}

function readKeyType(key: COSEKey): number {
  try {
    const integer = key.key_type().as_int();
    if (integer === undefined) throw new TypeError("Key Type is not an integer");
    return Number(integer.to_str());
  } catch {
    throw new Error("Failed to retrieve Key Type.");
  }
}

function readPublicKey(key: COSEKey): PublicKey {
  try {
    const value = key.header(Label.from_ec_key(ECKey.X));
    if (value?.kind !== "bytes") throw new TypeError("Public key header is not bytes");
    return PublicKey.from_bytes(value.value);
  } catch {
    throw new Error("No public key found.");
  }
}

function readPayload(coseSign1: COSESign1): string {
  try {
    const payload = coseSign1.payload();
    if (payload === undefined) throw new TypeError("Payload is missing");
    return bytesToHex(payload);
  } catch {
    throw new Error("No payload found.");
  }
}

export const CIP8Message = Object.freeze({
  signData,
  verifyData,
});
