import {
  assertByteLength,
  copyBytes,
} from "@xray-network/cardano-core";
import type { CborValue } from "@xray-network/cardano-core";
import { blake2b224 } from "@xray-network/cardano-crypto";

import {
  AlgorithmId,
  COSEKey,
  COSESign,
  COSESign1,
  COSESignatures,
  CurveType,
  ECKey,
  Headers,
  KeyOperation,
  KeyType,
  Label,
  Labels,
  SigContext,
  SigStructure,
} from "./model.js";

function cloneHeaders(value: Headers): Headers {
  return Headers.from_cbor_bytes(value.to_cbor_bytes());
}

function bytes(value: Uint8Array): CborValue {
  return {
    kind: "bytes",
    value: copyBytes(value),
    encoding: { kind: "definite", width: 0 },
  };
}

export class COSESign1Builder {
  #headers: Headers;
  #payload: Uint8Array;
  #externalAad: Uint8Array | undefined;
  readonly #isPayloadExternal: boolean;
  #hashed = false;

  private constructor(headers: Headers, payload: Uint8Array, isPayloadExternal: boolean) {
    const unprotected = headers.unprotected();
    unprotected.set_header(Label.new_text("hashed"), { kind: "boolean", value: false });
    this.#headers = Headers.new(headers.protected(), unprotected);
    this.#payload = copyBytes(payload);
    this.#isPayloadExternal = isPayloadExternal;
  }

  public static new(
    headers: Headers,
    payload: Uint8Array,
    isPayloadExternal: boolean,
  ): COSESign1Builder {
    return new COSESign1Builder(cloneHeaders(headers), payload, isPayloadExternal);
  }

  public hash_payload(): void {
    if (this.#hashed) return;
    this.#hashed = true;
    this.#payload = blake2b224(this.#payload);
    const unprotected = this.#headers.unprotected();
    unprotected.set_header(Label.new_text("hashed"), { kind: "boolean", value: true });
    this.#headers = Headers.new(this.#headers.protected(), unprotected);
  }

  public set_external_aad(value: Uint8Array): void {
    this.#externalAad = copyBytes(value);
  }

  public make_data_to_sign(): SigStructure {
    return SigStructure.new(
      SigContext.Signature1,
      this.#headers.protected(),
      this.#externalAad ?? new Uint8Array(),
      this.#payload,
    );
  }

  public build(signature: Uint8Array): COSESign1 {
    return COSESign1.new(
      this.#headers,
      this.#isPayloadExternal ? undefined : this.#payload,
      signature,
    );
  }
}

export class COSESignBuilder {
  readonly #headers: Headers;
  #payload: Uint8Array;
  #externalAad: Uint8Array | undefined;
  readonly #isPayloadExternal: boolean;
  #hashed = false;

  private constructor(headers: Headers, payload: Uint8Array, isPayloadExternal: boolean) {
    this.#headers = cloneHeaders(headers);
    this.#payload = copyBytes(payload);
    this.#isPayloadExternal = isPayloadExternal;
  }

  public static new(
    headers: Headers,
    payload: Uint8Array,
    isPayloadExternal: boolean,
  ): COSESignBuilder {
    return new COSESignBuilder(headers, payload, isPayloadExternal);
  }

  public hash_payload(): void {
    if (this.#hashed) return;
    this.#hashed = true;
    this.#payload = blake2b224(this.#payload);
  }

  public set_external_aad(value: Uint8Array): void {
    this.#externalAad = copyBytes(value);
  }

  public make_data_to_sign(): SigStructure {
    return SigStructure.new(
      SigContext.Signature,
      this.#headers.protected(),
      this.#externalAad ?? new Uint8Array(),
      this.#payload,
    );
  }

  public build(signatures: COSESignatures): COSESign {
    return COSESign.new(
      this.#headers,
      this.#isPayloadExternal ? undefined : this.#payload,
      signatures,
    );
  }
}

export class EdDSA25519Key {
  readonly #publicKey: Uint8Array;
  #forSigning = false;
  #forVerifying = false;

  private constructor(publicKey: Uint8Array) {
    assertByteLength("EdDSA25519Key public key", publicKey, 32);
    this.#publicKey = copyBytes(publicKey);
  }

  public static new(publicKey: Uint8Array): EdDSA25519Key {
    return new EdDSA25519Key(publicKey);
  }

  public is_for_signing(): void {
    this.#forSigning = true;
  }

  public is_for_verifying(): void {
    this.#forVerifying = true;
  }

  public build(): COSEKey {
    const key = COSEKey.new(Label.from_key_type(KeyType.OKP));
    key.set_header(
      Label.from_ec_key(ECKey.CRV),
      Label.from_curve_type(CurveType.Ed25519).toNode(),
    );
    key.set_header(Label.from_ec_key(ECKey.X), bytes(this.#publicKey));
    key.set_algorithm_id(Label.from_algorithm_id(AlgorithmId.EdDSA));
    if (this.#forSigning || this.#forVerifying) {
      const operations = Labels.new();
      if (this.#forSigning) operations.add(Label.from_key_operation(KeyOperation.Sign));
      if (this.#forVerifying) operations.add(Label.from_key_operation(KeyOperation.Verify));
      key.set_key_ops(operations);
    }
    return key;
  }
}
